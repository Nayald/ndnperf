/*    
Copyright (C) 2015-2016  Xavier MARCHAL

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/security/sec-tpm-file.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/lp/nack.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <crypto++/rsa.h>
#include <crypto++/dsa.h>
#include <crypto++/eccrypto.h>
#include <crypto++/sha.h>
#include <crypto++/osrng.h>
#include <crypto++/base64.h>
#include <crypto++/files.h>

#include <iostream>
#include <fstream>
#include <csignal>
#include <ctime>
#include <string>
#include <thread>

#include "blockingconcurrentqueue.h"

using namespace ndn;

// global constants and variables
namespace global {
    static const int DEFAULT_THREAD_COUNT = boost::thread::hardware_concurrency();
    static const int DEFAULT_CHUNK_SIZE = 8192;
    static const int DEFAULT_FRESHNESS = 0;
    static const int DEFAULT_SIGNATURE_TYPE = 1;
    static const int DEFAULT_RSA_SIGNATURE = 2048;
    static const int DEFAULT_ECDSA_SIGNATURE = 256;

    static bool stop = false;
}

class Server {
private:
    bool _stop;
    Face _face;
    //boost::asio::io_service *ios;
    Name _prefix;
    KeyChain _keyChain;
    moodycamel::BlockingConcurrentQueue<Name> _queue;
    //std::thread *_threads;
    boost::thread_group _threads;
    int *_log_vars; // may be less accurate than atomic variables but no sync required
    Block _content;
    int _thread_count, _payload_size, _key_type, _key_size;
    time::milliseconds _freshness;
    Name _defaultIdentity, _identity, _keyName, _certName;

    //vars for the customSign function
    bool _override;
    Signature _sig;
    shared_ptr<PublicKey> _pubkey;
    // I store directly the signers but it may be better to just store the private keys or the path to the keys
    CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Signer _rsaSigner;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer _ecdsaSigner;

    void gen_random(char *s, const int len) {
        static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        for (int i = 0; i < len; ++i) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        s[len] = 0;
    }

    // up to 20% faster compare to default sign() function with 32 threads on bi Xeon E5-2630v3 server and ECDSA
    void customSign(Data &data) {
        data.setSignature(_sig);
        EncodingBuffer enc;
        data.wireEncode(enc, true);
        if (_keyName == KeyChain::DIGEST_SHA256_IDENTITY)
            data.wireEncode(enc, Block(tlv::SignatureValue, crypto::sha256(enc.buf(), enc.size())));
        else {
            CryptoPP::AutoSeededRandomPool rng;
            OBufferStream os;
            switch (_pubkey->getKeyType()) {
                case KEY_TYPE_RSA:
                    //seems to not be necessary
                    //CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Signer rsaSignerCopy = rsaSigner;

                    CryptoPP::StringSource(enc.buf(), enc.size(), true,
                                           new CryptoPP::SignerFilter(rng, _rsaSigner, new CryptoPP::FileSink(os)));
                    data.wireEncode(enc, Block(tlv::SignatureValue, os.buf()));
                    break;
                case KEY_TYPE_ECDSA:
                    // if not present -> segfault with concurrency
                    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer ecdsaSignerCopy = _ecdsaSigner;

                    CryptoPP::StringSource(enc.buf(), enc.size(), true,
                                           new CryptoPP::SignerFilter(rng, ecdsaSignerCopy,
                                                                      new CryptoPP::FileSink(os)));
                    uint8_t buf[200];
                    size_t bufSize = CryptoPP::DSAConvertSignatureFormat(buf, 200, CryptoPP::DSA_DER, os.buf()->buf(),
                                                                         os.buf()->size(), CryptoPP::DSA_P1363);
                    shared_ptr<Buffer> sigBuffer = make_shared<Buffer>(buf, bufSize);
                    data.wireEncode(enc, Block(tlv::SignatureValue, sigBuffer));
                    break;
            }
        }
    }

public:
    Server(const char *prefix, int key_type, int key_size, int thread_count, int freshness, int payload_size,
           bool override/*, boost::asio::io_service *ios*/)
        : _stop(false)
        , _face()
        , _prefix(prefix)
        , _key_type(key_type)
        , _key_size(key_size)
        , _thread_count(thread_count)
        , _freshness(freshness)
        , _payload_size(payload_size)
        , _override(override)
    {
        if (_key_type < 0) {
            _key_type = 1;
        }
        if (_key_type == 1 && _key_size <= 0) {
            _key_size = global::DEFAULT_RSA_SIGNATURE;
        }
        if (_key_type == 3 && _key_size <= 0) {
            _key_size = global::DEFAULT_ECDSA_SIGNATURE;
        }
        if (_key_type > 0) { // index of NDN Signature: http://named-data.net/doc/NDN-TLV/current/signature.html
            _identity = Name(prefix);
            _keyChain.createIdentity(_identity);
            if (!override) {
                // signature take less time with default sign() function ,
                // xeon E5-2630v3 with RSA-2048: ptime with default 3500us with certName 4100us, with identity 6500us
                _defaultIdentity = _keyChain.getDefaultIdentity();
                _keyChain.setDefaultIdentity(_identity);
                std::cout << "Set default identity to " << _identity << " instead of " << _defaultIdentity << std::endl;
            } else {
                std::cout << "The sign function will be override" << std::endl;
            }

            std::string digest;
            CryptoPP::SHA256 hash;
            CryptoPP::ByteQueue bytes;
            switch (_key_type) {
                default:
                case 1: {
                    std::cout << "Generating " << _key_size << "bits RSA key pair" << std::endl;
                    _keyName = _keyChain.generateRsaKeyPairAsDefault(_identity, false, _key_size);
                    CryptoPP::StringSource src(_keyName.toUri(), true, new CryptoPP::HashFilter(hash,
                                                                                                new CryptoPP::Base64Encoder(
                                                                                                        new CryptoPP::StringSink(
                                                                                                                digest))));
                    boost::algorithm::trim(digest);
                    std::replace(digest.begin(), digest.end(), '/', '%');
                    boost::filesystem::path path =
                            boost::filesystem::path(getenv("HOME")) / ".ndn" / "ndnsec-tpm-file" / (digest + ".pri");

                    CryptoPP::FileSource file(path.string().c_str(), true, new CryptoPP::Base64Decoder);
                    file.TransferTo(bytes);
                    bytes.MessageEnd();
                    CryptoPP::RSA::PrivateKey privateRsaKey;
                    privateRsaKey.Load(bytes);
                    _rsaSigner = CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Signer(privateRsaKey);
                    break;
                }
                case 3: {
                    std::cout << "Generating " << _key_size << "bits ECDSA key pair" << std::endl;
                    _keyName = _keyChain.generateEcdsaKeyPairAsDefault(_identity, false, _key_size);
                    CryptoPP::StringSource src(_keyName.toUri(), true, new CryptoPP::HashFilter(hash,
                                                                                                new CryptoPP::Base64Encoder(
                                                                                                        new CryptoPP::StringSink(
                                                                                                                digest))));
                    boost::algorithm::trim(digest);
                    std::replace(digest.begin(), digest.end(), '/', '%');
                    boost::filesystem::path path =
                            boost::filesystem::path(getenv("HOME")) / ".ndn" / "ndnsec-tpm-file" / (digest + ".pri");

                    CryptoPP::FileSource file(path.string().c_str(), true, new CryptoPP::Base64Decoder);
                    file.TransferTo(bytes);
                    bytes.MessageEnd();
                    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateEcdsaKey;
                    privateEcdsaKey.Load(bytes);
                    _ecdsaSigner = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer(privateEcdsaKey);
                    break;
                }
            }
            shared_ptr<IdentityCertificate> certificate = _keyChain.selfSign(_keyName);
            _keyChain.addCertificate(*certificate);
            _keyChain.setDefaultCertificateNameForKey(certificate->getName());
            _certName = certificate->getName();

            _pubkey = _keyChain.getPublicKeyFromTpm(certificate->getPublicKeyName());
            SignatureInfo sigInfo = KeyChain::DEFAULT_SIGNING_INFO.getSignatureInfo();
            sigInfo.setSignatureType((tlv::SignatureTypeValue) _key_type);
            sigInfo.setKeyLocator(KeyLocator(certificate->getName().getPrefix(-1)));
            _sig = Signature(sigInfo);

            Name defaultIdentityName = _keyChain.getDefaultIdentity();
            Name defaultKeyName = _keyChain.getDefaultKeyNameForIdentity(_identity);
            Name defaultCertName = _keyChain.getDefaultCertificateNameForKey(defaultKeyName);
            std::cout << "+ Using identity: " << defaultIdentityName << "\n"
                      << "+ Using key: " << defaultKeyName << "\n"
                      << "+ Using certificate: " << defaultCertName << std::endl;
        } else {
            std::cout << "Using SHA-256 signature" << std::endl;
            _keyName = KeyChain::DIGEST_SHA256_IDENTITY;
            SignatureInfo sigInfo = KeyChain::DEFAULT_SIGNING_INFO.getSignatureInfo();
            sigInfo.setSignatureType(tlv::DigestSha256);
            _sig = Signature(sigInfo);
        }

        //threads = new std::thread[thread_count + 1]; // N sign threads + 1 display thread
        _log_vars = new int[thread_count * 3](); // [thread_count][1: payload sum, 2: packet count, 3: ptime]

        // build once for all the data carried by the packets
        char chararray[payload_size];
        gen_random(chararray, payload_size);
        shared_ptr<Buffer> buf = make_shared<Buffer>(&chararray[0], payload_size);
        _content = Block(tlv::Content, buf);
        std::cout << "Payload size = " << _content.value_size() << " Bytes" << std::endl;
        std::cout << "Freshness = " << freshness << " ms" << std::endl;
    }

    ~Server() {}

    void start() {
        _face.setInterestFilter(_prefix, bind(&Server::on_interest, this, _2),
                                bind(&Server::on_register_failed, this));

        for (int i = 0; i < _thread_count; ++i) {
            //threads[i] = std::thread(&Server::process, this, &log_vars[i * 3]);
            _threads.create_thread(boost::bind(&Server::process, this, &_log_vars[i * 3]));
        }
        std::cout << "Initialize " << _threads.size() << " threads" << std::endl;
        //threads[thread_count] = std::thread(&Server::display, this);
        _threads.create_thread(boost::bind(&Server::display, this));
        _threads.create_thread(boost::bind(&boost::asio::io_service::run, &_face.getIoService()));

    }

    void stop() {
        // stop the threads
        _stop = true;
        Name name(_prefix);
        name.append("dummy");
        _face.getIoService().stop();
        for (int i = 0; i < _thread_count; ++i) {
            _queue.enqueue(name);
        }
        _threads.join_all();

        // clean up
        if (_key_type > 0) {
            if (!_override) {
                std::cout << "Restoring default identity to " << _defaultIdentity << "... " << std::flush;
                _keyChain.setDefaultIdentity(_defaultIdentity);
                std::cout << (_keyChain.getDefaultIdentity() == _defaultIdentity ? "done" : "failed") << std::endl;
            }
            std::cout << "Deleting generated certificate... " << std::flush;
            _keyChain.deleteCertificate(_certName);
            std::cout << (!_keyChain.doesCertificateExist(_certName) ? "done" : "failed") << std::endl;
            std::cout << "Deleting generated key pair... " << std::flush;
            _keyChain.deleteKey(_keyName);
            std::cout << (!_keyChain.doesPublicKeyExist(_keyName) ? "done" : "failed") << std::endl;
            std::cout << "Deleting generated identity... " << std::flush;
            _keyChain.deleteIdentity(_identity);
            std::cout << (!_keyChain.doesIdentityExist(_identity) ? "done" : "failed") << std::endl;
        }
    }

    void process(int *const i) {
        Name name;
        std::ifstream file;
        char buffer[_payload_size];
        while (!_stop) {
            _queue.wait_dequeue(name);
            auto start = std::chrono::steady_clock::now();

            auto data = make_shared<Data>(name);
            data->setFreshnessPeriod(_freshness);

            if (name.get(_prefix.size()).toUri() == "download") {
                file.open(name.getSubName(_prefix.size() + 1).getPrefix(-1).toUri().erase(0,1),
                          std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
                if (file.is_open()) {
                    int max_seg_num = ((int) file.tellg() - 1) / _payload_size;
                    file.seekg(stoi(name.get(-1).toUri()) * _payload_size, std::ios_base::beg);
                    auto size = file.read(buffer, _payload_size).gcount();
                    data->setContent(reinterpret_cast<uint8_t *>(buffer), size);
                    data->setFinalBlockId(Name::Component(std::to_string(max_seg_num)));
                    i[0] += size;
                }
                file.close();
            } else {
                data->setContent(_content);
                i[0] += _payload_size;
            }

            if (!_override) {
                if (_key_type > 0)_keyChain.sign(*data);
                else _keyChain.signWithSha256(*data);
            } else customSign(*data);

            _face.put(*data);

            ++i[1];
            i[2] += std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count();
        }
    }

    void display() {
        std::time_t time;
        char mbstr[32];
        int lv[6]; // [1: payload sum, 2: packet count, 3: ptime][1: new, 2: last]
        while (!_stop) {
            // accumulate value and compare with last
            for (int i = 0; i < 6; i += 2) {
                lv[i + 1] = -lv[i];
                lv[i] = 0;
            }
            for (int i = 0; i < _threads.size() - 1; ++i) {
                for (int j = 0; j < 3; ++j) {
                    lv[2 * j] += _log_vars[3 * i + j];
                }
            }
            for (int i = 0; i < 6; i += 2) {
                lv[i + 1] += lv[i];
            }
            lv[1] >>= 8; // in kilobits per sec each 2s (10 + 1 - 3), in decimal 8/(1024*2);
            lv[5] /= lv[3] != 0 ? lv[3] : -1; // negative value if unusual
            lv[3] >>= 1; // per sec each 2s
            time = std::time(NULL);
            std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
            std::cout << mbstr << lv[1] << " Kbps( " << lv[3] << " pkt/s) - ptime= " << lv[5] << " us" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void on_interest(const Interest &interest) {
        // it may be me but when i use multiple thread with _face.processEvent(),
        // the sum of the workloads is equivalent to 1 running thread
        _queue.enqueue(interest.getName());
    }

    void on_register_failed() {
        exit(1);
    }
};

void signalHandler(int signum) {
    std::cout << "Catch interruption!" << std::endl;
    global::stop = true;
}

int main(int argc, char *argv[]) {
    // default values
    int thread_count = global::DEFAULT_THREAD_COUNT;
    int chunk_size = global::DEFAULT_CHUNK_SIZE;
    int freshness = global::DEFAULT_FRESHNESS;
    int key_type = global::DEFAULT_SIGNATURE_TYPE;
    int key_size = 0;
    bool override = false;
    const char *prefix = "/throughput";

    // check parameters
    for (int i = 1; i < argc; i += 2) {
        switch (argv[i][1]) {
            case 'p':
                prefix = argv[i + 1];
                break;
            case 's':
                if (atoi(argv[i + 1]) == 0)key_type = 0;
                else if (atoi(argv[i + 1]) == 1)key_type = 1;
                else if (atoi(argv[i + 1]) == 3)key_type = 3;
                break;
            case 'k':
                if (key_type > 0) {
                    if (key_type == 1 && atoi(argv[i + 1]) >= 512)
                        key_size = atoi(argv[i + 1]);
                    else if (key_type == 3 && atoi(argv[i + 1]) >= 160)
                        key_size = atoi(argv[i + 1]);
                }
                break;
            case 't':
                if (atoi(argv[i + 1]) >= 0)
                    thread_count = atoi(argv[i + 1]);
                break;
            case 'c':
                if (atoi(argv[i + 1]) >= 0)
                    chunk_size = atoi(argv[i + 1]);
                break;
            case 'f':
                if (atoi(argv[i + 1]) >= 0)
                    freshness = atoi(argv[i + 1]);
                break;
            case 'x':
                override = true;
                break;
            case 'h':
            default:
                std::cout << "usage: ./ndnperfserver [options...]\n\n"
                          << "-p prefix\t(default = /throughput)\n"
                          << "-s sign_type\t0=SHA,1=RSA,3=ECDSA (default = 1)\n"
                          << "-k key_size\tlimited by the lib, RSA={1024,2048} and ECDSA={256,384}\n"
                          << "-t thread_count\t(default = CPU core number)\n"
                          << "-c chunk_size\t(default = 8192)\n"
                          << "-f freshness\tin milliseconds (default = 0)\n"
                          << "-x\t\toverride the ndn sign function with ndnperf sign function\n"
                          << "-h\t\tdisplay this help message\n"
                          << std::endl;
                return 0;
        }
    }
    //boost::thread_group thread_pool;
    //boost::asio::io_service pool_service;
    //boost::asio::io_service::work pool_work(pool_service);
    //for(int i=0;i<thread_count;i++)thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &pool_service));

    Server server(prefix, key_type, key_size, thread_count, freshness, chunk_size, override/*,&pool_service*/);
    signal(SIGINT, signalHandler);
    server.start();
    while (!global::stop) {
        sleep(10);
    }
    server.stop();
    return 0;
}
