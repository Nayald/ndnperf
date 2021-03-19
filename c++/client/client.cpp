#include "client.h"
#include <iostream>

Client::Client(const std::string &prefix, size_t window, const std::string &file_path)
        : Module(1)
        , _face(std::make_shared<ndn::TcpTransport>("127.0.0.1", "6363"), _ios)
        , _prefix(prefix)
        , _window(window)
        , _timer(_ios)
        , _file_path(file_path) {

}

void Client::run() {
    std::cout << "Client start with window = " << _window << std::endl;
    if (!_file_path.empty()) {
        std::string file_name = _file_path.substr(_file_path.find_last_of('/') + 1);
        _file.open(file_name, std::ofstream::out | std::ofstream::binary);
        download = true;
    }
    for (_current_packet; _current_packet < _window; ++_current_packet) {
        ndn::Name name(_prefix);
        if (download) {
            name.append("download").append(_file_path);
        } else {
            name.append("benchmark");
        }
        ndn::Interest interest(name.appendSegment(_current_packet));
        interest.setMustBeFresh(true);
        if (download) {
            _face.expressInterest(interest, boost::bind(&Client::onDataFile, this, _current_packet, _2),
                                  boost::bind(&Client::onNack, this, _1, _2),
                                  boost::bind(&Client::onTimeoutFile, this, _current_packet, _1, 5));
        } else {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            _face.expressInterest(interest, boost::bind(&Client::onData, this, _1, _2, start),
                                  boost::bind(&Client::onNack, this, _1, _2),
                                  boost::bind(&Client::onTimeout, this, _1, 5, start));
        }
    }
    waitNextDisplay();
}

void Client::waitNextDisplay() {
    _timer.expires_from_now(boost::posix_time::seconds(2));
    _timer.async_wait(boost::bind(download ? &Client::displayFile : &Client::display, this));
}

void Client::onNack(const ndn::Interest &interest, const ndn::lp::Nack &nack) {
    std::cout << "Nack received: " << nack.getReason() << std::endl;
    if(_exit_on_nack) {
        exit(-1);
    }
    // todo: avoid burst when resending interests after nack
    std::cout << "Resending interest with name " << interest.getName() << std::endl;
    ndn::Interest i(interest);
    i.refreshNonce();
    if (download) {
        const uint64_t current_packet = interest.getName().get(-1).toSegment();
        _face.expressInterest(i, boost::bind(&Client::onDataFile, this, current_packet, _2),
                              boost::bind(&Client::onNack, this, _1, _2),
                              boost::bind(&Client::onTimeoutFile, this, current_packet, _1, 5));
    } else {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        _face.expressInterest(i, boost::bind(&Client::onData, this, _1, _2, start),
                              boost::bind(&Client::onNack, this, _1, _2),
                              boost::bind(&Client::onTimeout, this, _1, 5, start));
    }     
}

// benchmark -----------------------------------------------------------------------------------------------------------

void Client::onData(const ndn::Interest &interest, const ndn::Data &data, std::chrono::steady_clock::time_point start) {
    _rtt += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
    if (_first) {
        //std::cout << "Server signature type = " << data.getSignature().getType() << std::endl;
        std::cout << "Server signature type = " << data.getSignatureType() << std::endl;
        std::cout << "Server packet size = " << data.getContent().value_size() << std::endl;
        _first = false;
    }
    ndn::Interest i(ndn::Name(_prefix).append("benchmark").append(std::to_string(_current_packet++)));
    i.setMustBeFresh(true);
    start = std::chrono::steady_clock::now();
    _face.expressInterest(i, boost::bind(&Client::onData, this, _1, _2, start),
                          boost::bind(&Client::onNack, this, _1, _2),
                          boost::bind(&Client::onTimeout, this, _1, 5, start));
    ++_pkt_count;
    _payload_size += data.getContent().value_size();
}

void Client::onTimeout(const ndn::Interest &interest, int n, std::chrono::steady_clock::time_point start) {
    if (n > 0) {
        ndn::Interest i(interest);
        i.refreshNonce();
        _face.expressInterest(i, boost::bind(&Client::onData, this, _1, _2, start),
                              boost::bind(&Client::onNack, this, _1, _2),
                              boost::bind(&Client::onTimeout, this, _1, n - 1, start));
    } else {
        std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
        exit(1);
    }
}

void Client::display() {
    static char mbstr[32];

    time_t time = std::time(NULL);
    std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
    std::cout << mbstr << (_payload_size >>= 8) << " Kbps ( " << (_pkt_count >>= 1) << " pkt/s) - latency = " << (_rtt /= _pkt_count != 0 ? _pkt_count : -1) << " us" << std::endl;
    _rtt = 0;
    _payload_size = 0;
    _pkt_count = 0;
    waitNextDisplay();
}

// download ------------------------------------------------------------------------------------------------------------

void Client::onDataFile(size_t segment, const ndn::Data &data) {
    //std::cout << data << std::endl;
    //_max_segment = data.getFinalBlockId().toSegment();
    _max_segment = data.getFinalBlock()->toSegment();
    _payload_size += data.getContent().value_size();
    _pending_segments[segment] = data;
    while (_pending_segments.find(_current_segment) != _pending_segments.end()) {
        _file.write(reinterpret_cast<const char *>(_pending_segments.at(_current_segment).getContent().value()),
                    _pending_segments.at(_current_segment).getContent().value_size());
        _pending_segments.erase(_current_segment);
        ++_current_segment;
    }
    if (_current_segment > _max_segment) {
        _file.close();
        std::cout << "download completed!" << std::endl;
        exit(0);
    }
    if (_current_packet <= _max_segment) {
        ndn::Interest interest(data.getName().getPrefix(-1).appendSegment(_current_packet));
        interest.setMustBeFresh(true);
        _face.expressInterest(interest, boost::bind(&Client::onDataFile, this, _current_packet, _2),
                              boost::bind(&Client::onNack, this, _1, _2),
                              boost::bind(&Client::onTimeoutFile, this, _current_packet, _1, 5));
        ++_current_packet;
    }
}

void Client::onTimeoutFile(int segment, const ndn::Interest &interest, int n) {
    if (n > 0) {
        ndn::Interest i(interest);
        i.refreshNonce();
        _face.expressInterest(i, boost::bind(&Client::onDataFile, this, segment, _2),
                              boost::bind(&Client::onNack, this, _1, _2),
                              boost::bind(&Client::onTimeoutFile, this, segment, _1, n - 1));
    } else {
        std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
        exit(1);
    }
}

void Client::displayFile() {
    static char mbstr[32];

    time_t time = std::time(NULL);
    std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
    std::cout << mbstr << _current_segment - 1 << "/" << _max_segment << " completed - "
              << (_payload_size >>= 6) << " Kbps" << std::endl;
    _payload_size = 0;
    waitNextDisplay();
}

void Client::setExitOnNack(bool state) {
    _exit_on_nack = state;
}