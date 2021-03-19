/*    
Copyright (C) 2015-2017  Xavier MARCHAL

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
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/encoding/buffer.hpp>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>

#include "blockingconcurrentqueue.h"

using namespace ndn;

// global constants and variables
namespace global {
    const size_t DEFAULT_THREAD_COUNT = boost::thread::hardware_concurrency();
    const char *DEFAULT_PREFIX = "/throughput";
    const tlv::SignatureTypeValue DEFAULT_SIGNATURE_TYPE = tlv::DigestSha256;
    const size_t DEFAULT_RSA_KEY_SIZE = 2048;
    const size_t DEFAULT_EC_KEY_SIZE = 256;
    const size_t DEFAULT_CHUNK_SIZE = 8192;
    const size_t DEFAULT_FRESHNESS = 0;
}

class Server {
private:
    void gen_random(char *s, size_t len) {
        static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        for (int i = 0; i < len; ++i) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        s[len] = 0;
    }

    bool _stop = false;

    const size_t _concurrency;
    boost::thread_group _thread_pool;

    const Name _prefix;
    //std::string remote_ip;
    //std::string remote_port;
    Face _face;
    KeyChain _keychain;
    security::Identity _identity;
    const tlv::SignatureTypeValue _key_type;
    size_t _key_size;
    security::Key _key;

    moodycamel::BlockingConcurrentQueue<std::pair<Name, std::chrono::steady_clock::time_point>> _queue;

    // vars for pre computed part of Data packets
    Block _content;
    const size_t _payload_size;
    const time::milliseconds _freshness;

    // array for statistics
    int *_log_vars; // may be less accurate than atomic variables but no sync required

public:
    Server(size_t concurrency, const char *prefix, tlv::SignatureTypeValue key_type, size_t key_size,
           size_t payload_size, size_t freshness)
        : _prefix(prefix)
        , _face()
        , _key_type(key_type)
        , _key_size(key_size)
        , _concurrency(concurrency)
        , _freshness(freshness)
        , _payload_size(payload_size) {
        if (_key_type != tlv::DigestSha256) {
            auto it = _keychain.getPib().getIdentities().find(_prefix);
            _identity = it != _keychain.getPib().getIdentities().end() ? *it : _keychain.createIdentity(prefix);

            switch (_key_type) {
                default:
                case tlv::SignatureSha256WithRsa: {
                    if(_key_size < 1024) {
                        _key_size = global::DEFAULT_RSA_KEY_SIZE;
                    }
                    std::cout << "Generating new " << _key_size << " bits RSA key pair" << std::endl;
                    _key = _keychain.createKey(_identity, RsaKeyParams(_key_size));
                    break;
                }
                case tlv::SignatureSha256WithEcdsa: {
                    if(_key_size != 256 && _key_size != 384) {
                        _key_size = global::DEFAULT_EC_KEY_SIZE;
                    }
                    std::cout << "Generating new " << _key_size << " bits ECDSA key pair" << std::endl;
                    _key = _keychain.createKey(_identity, EcKeyParams(_key_size));
                    break;
                }
            }

            std::cout << "Using key " << _key.getName() << "\n"
                      << _key.getDefaultCertificate() << std::endl;
        } else {
            std::cout << "Using SHA-256 signature" << std::endl;
        }

        _log_vars = new int[concurrency * 4](); // [concurrency][0: payload sum, 1: packet count, 2:qtime, 3: ptime]

        // build once for all the data carried by the packets (packets generated from files ignore this)
        char chararray[payload_size];
        gen_random(chararray, payload_size);
        shared_ptr<Buffer> buf = make_shared<Buffer>(&chararray[0], payload_size);
        _content = Block(tlv::Content, buf);
        std::cout << "Payload size = " << _content.value_size() << " Bytes" << std::endl;
        std::cout << "Freshness = " << freshness << " ms" << std::endl;
    }

    ~Server() = default;

    void start() {
        _face.setInterestFilter(_prefix, bind(&Server::on_interest, this, _2), bind(&Server::on_register_failed, this));

        for (int i = 0; i < _concurrency; ++i) {
            _thread_pool.create_thread(boost::bind(&Server::process, this, &_log_vars[i * 4]));
        }
        std::cout << "Start server with " << _concurrency << " signing threads" << std::endl;
        _thread_pool.create_thread(boost::bind(&Server::display, this));
        _thread_pool.create_thread(boost::bind(&Face::processEvents, &_face, time::milliseconds::zero(), false));
    }

    void stop() {
        // stop the threads
        std::cout << "Waiting for other threads... " << std::endl;
        _stop = true;
        Name name(_prefix);
        name.append("dummy");
        _face.getIoService().stop();
        for (int i = 0; i < _concurrency; ++i) {
            _queue.enqueue(std::make_pair(name, std::chrono::steady_clock::now()));
        }
        _thread_pool.join_all();

        // clean up
        if (_key_type > 0) {
            std::cout << "Deleting generated key... " << std::endl;
            _keychain.deleteKey(_identity, _key);
        }
    }

    void process(int *const i) {
        // thread must own its own keychain since RSA or ECDSA will segfault with 2+ threads
        KeyChain keychain;
        std::pair<Name, std::chrono::steady_clock::time_point> pair;
        std::ifstream file;
        char buffer[_payload_size];
        while (!_stop) {
            _queue.wait_dequeue(pair);
            Name name = pair.first;

            auto start = std::chrono::steady_clock::now();
            i[2] += std::chrono::duration_cast<std::chrono::microseconds>(start - pair.second).count();

            auto data = make_shared<Data>(name);
            data->setFreshnessPeriod(_freshness);

            if (name.get(_prefix.size()).toUri() == "download") {
                file.open(name.getSubName(_prefix.size() + 1).getPrefix(-1).toUri().erase(0,1),
                          std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
                if (file.is_open()) {
                    long max_seg_num = ((long)file.tellg() - 1) / _payload_size;
                    file.seekg(name.get(-1).toSegment() * _payload_size, std::ios_base::beg);
                    long size = file.read(buffer, _payload_size).gcount();
                    data->setContent((uint8_t*)buffer, size);
                    data->setFinalBlock(ndn::Name::Component::fromSegment(max_seg_num));
                    i[0] += size;
                }
                file.close();
            } else {
                data->setContent(_content);
                i[0] += _payload_size;
            }

            if (_key_type != tlv::DigestSha256) {
                keychain.sign(*data, security::SigningInfo(_key));
            } else {
                // sign with DigestSha256
                keychain.sign(*data, security::SigningInfo(security::SigningInfo::SIGNER_TYPE_SHA256));
            }
            _face.put(*data);

            ++i[1];
            i[3] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
        }
    }

    void display() {
        std::time_t time;
        char mbstr[32];
        int log_vars[8] = {}; // [0: payload sum, 1: packet count, 2:qtime, 3: ptime][1: new, 2: last]
        while (!_stop) {
            // accumulate value and compare with last
            for (int i = 0; i < 8; i += 2) {
                log_vars[i + 1] = -log_vars[i];
                log_vars[i] = 0;
            }
            for (int i = 0; i < _concurrency; ++i) {
                for (int j = 0; j < 4; ++j) {
                    log_vars[2 * j] += _log_vars[4 * i + j];
                }
            }
            for (int i = 0; i < 8; i += 2) {
                log_vars[i + 1] += log_vars[i];
            }
            log_vars[1] >>= 8; // in kilobits per second each 2 seconds (10 + 1 - 3), in decimal 8/(1024*2);
            log_vars[5] /= log_vars[3] != 0 ? log_vars[3] : -1; // negative value if unusual
            log_vars[7] /= log_vars[3] != 0 ? log_vars[3] : -1; // negative value if unusual
            log_vars[3] >>= 1; // per second each 2 seconds
            time = std::time(NULL);
            std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
            std::cout << mbstr << log_vars[1] << " Kbps( " << log_vars[3] << " pkt/s) - qtime= " << log_vars[5] << " us, ptime= " << log_vars[7] << " us" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void on_interest(const Interest &interest) {
        // it may be me but when i use multiple threads with _face.processEvent(),
        // the sum of the workloads is equivalent to a single thread
        _queue.enqueue(std::make_pair(interest.getName(), std::chrono::steady_clock::now()));
    }

    void on_register_failed(){
        std::cerr << "Failed to register prefix " << _prefix << std::endl;
        std::exit(-1);
    }
};

static bool stop = false;
void signalHandler(int signum) {
    stop = true;
}

int main(int argc, char *argv[]) {
    // default values
    size_t concurrency = global::DEFAULT_THREAD_COUNT;
    const char *prefix = global::DEFAULT_PREFIX;
    tlv::SignatureTypeValue key_type = global::DEFAULT_SIGNATURE_TYPE;
    size_t key_size = 0;
    size_t payload_size = global::DEFAULT_CHUNK_SIZE;
    size_t freshness = global::DEFAULT_FRESHNESS;

    // check parameters
    for (int i = 1; i < argc; ++i) {
        switch (argv[i][1]) {
            case 'p':
                prefix = argv[++i];
                break;
            case 'k':
                ++i;
                if (strcmp(argv[i], "rsa") == 0)key_type = tlv::SignatureSha256WithRsa;
                else if (strcmp(argv[i], "ecdsa") == 0)key_type = tlv::SignatureSha256WithEcdsa;
                break;
            case 's':
                if (atoi(argv[++i]) >= 0)
                    key_size = atoi(argv[i]);
                break;
            case 't':
                if (atoi(argv[++i]) >= 0)
                    concurrency = atoi(argv[i]);
                break;
            case 'c':
                if (atoi(argv[++i]) >= 0)
                    payload_size = atoi(argv[i]);
                break;
            case 'f':
                if (atoi(argv[++i]) >= 0)
                    freshness = atoi(argv[i]);
                break;
            case 'h':
            default:
                std::cout << "usage: " << argv[0] << " [options...]\n\n"
                          << "-p prefix          prefix to register in the FIB (default = /throughput)\n"
                          << "-k key_type        key_type={sha, rsa, ecdsa} (default = sha)\n"
                          << "-s key_size        size of the key, rsa >= 1024, ecdsa={256, 384}\n"
                          << "-t thread_count    number of CPU used to handle Interests (default = logical core count)\n"
                          << "-c payload_size    size of the data carried by the packet (default = 8192)\n"
                          << "-f freshness       freshness of the Data in milliseconds (default = 0)\n"
                          << "-h                 display this help message\n"
                          << std::endl;
                return 0;
        }
    }

    Server server(concurrency, prefix, key_type, key_size, payload_size, freshness);
    signal(SIGINT, signalHandler);
    server.start();

    do {
        sleep(15);
    } while (!stop);

    server.stop();

    return 0;
}
