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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

using namespace ndn;

static const int DEFAULT_WINDOW = 32;

class Client {
private:
    Face _face;
    Name _prefix;
    
    size_t _window;
    bool _first = true;
    std::atomic<int> _count{0};
    std::atomic<int> _sum_size{0};
    std::atomic<int> _sum_rtt{0};
    std::thread *_thread;

    bool _download;
    size_t _current_packet = 0;
    size_t _current_segment = 0;
    size_t _max_segment = 0;
    std::map<int, Data> _pending_segments;
    std::ofstream _file;
    std::string _file_path;

public:
    Client(const char *prefix, int window, bool download, const char *file_path)
            : _face()
            , _prefix(prefix)
            , _window(window)
            , _download(download)
            , _file_path(file_path) {
            
    }

    ~Client() {
    
    }

    void run() {
        std::cout << "Client start with window = " << _window << std::endl;
        if (_download) {
            std::string file_name = std::string(_file_path);
            file_name = file_name.substr(file_name.find_last_of("/") + 1);
            _file.open(file_name, std::ofstream::out | std::ofstream::binary);
            _thread = new std::thread(&Client::display_file, this);
        } else {
            _thread = new std::thread(&Client::display, this);
        }
        for (_current_packet; _current_packet < _window; ++_current_packet) {
            Name name = Name(_prefix);
            if (_download) {
                name.append("download").append(_file_path);
            }
            else {
                name.append("benchmark");
            }
            Interest interest = Interest(name.appendSegment(_current_packet), time::milliseconds(4000));
            interest.setMustBeFresh(true);
            if (_download) {
                _face.expressInterest(interest, bind(&Client::on_file, this, _current_packet, _2),
                                      bind(&Client::on_nack, this, _1, _2),
                                      bind(&Client::on_timeout_file, this, _current_packet, _1, 5));
            } else {
                std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
                _face.expressInterest(interest, bind(&Client::on_data, this, _1, _2, start),
                                      bind(&Client::on_nack, this, _1, _2),
                                      bind(&Client::on_timeout, this, _1, 5, start));
            }
        }
        _face.processEvents();
    }

    void display() {
        std::time_t time;
        char mbstr[28];
        int sum = 0, cpt = 0, rtt = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            sum = _sum_size.exchange(0);
            cpt = _count.exchange(0);
            rtt = _sum_rtt.exchange(0);
            rtt /= cpt != 0 ? cpt : -1;
            sum >>= 8; // in kilobits per sec (10 + 1 - 3), in decimal 8/(1024*2);
            cpt >>= 1; // per sec;
            time = std::time(NULL);
            std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
            std::cout << mbstr << sum << " Kbps ( " << cpt << " pkt/s) - latency = " << rtt << " us" << std::endl;
        }
    }

    void display_file() {
        std::time_t time;
        char mbstr[28];
        int sum = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            time = std::time(NULL);
            std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
            std::cout << mbstr << _current_segment - 1 << "/" << _max_segment << " completed - "
                      << (_sum_size.exchange(0) >> 6) << " Kbps" << std::endl;
        }
    }

    void on_data(const Interest &interest, const Data &data, std::chrono::steady_clock::time_point start) {
        _sum_rtt.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start).count());
        if (_first) {
            std::cout << "Server signature type = " << data.getSignature().getType() << std::endl;
            std::cout << "Server packet size = " << data.getContent().value_size() << std::endl;
            _first = false;
        }
        Interest i = Interest(Name(_prefix).append("benchmark").append(std::to_string(_current_packet++)),
                              time::milliseconds(4000)).setMustBeFresh(true);
        start = std::chrono::steady_clock::now();
        _face.expressInterest(i, bind(&Client::on_data, this, _1, _2, start),
                              bind(&Client::on_nack, this, _1, _2),
                              bind(&Client::on_timeout, this, _1, 5, start));
        _count.fetch_add(1);
        _sum_size.fetch_add(data.getContent().value_size());
    }

    void on_file(size_t segment, const Data &data) {
        _max_segment = data.getFinalBlockId().toSegment();
        _sum_size.fetch_add(data.getContent().value_size());
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
            Interest i = Interest(data.getName().getPrefix(-1).appendSegment(_current_packet),
                                  time::milliseconds(4000)).setMustBeFresh(true);
            _face.expressInterest(i, bind(&Client::on_file, this, _current_packet, _2),
                                  bind(&Client::on_nack, this, _1, _2),
                                  bind(&Client::on_timeout_file, this, _current_packet, _1, 5));
            ++_current_packet;
        }
    }

    void on_nack(const Interest &interest, const lp::Nack nack) {
        std::cout << "Nack receive : " << nack.getReason() << std::endl;
        exit(-1);
    }

    void on_timeout(const Interest &interest, int n, std::chrono::steady_clock::time_point start) {
        if (n > 0) {
            Interest i(interest);
            i.refreshNonce();
            _face.expressInterest(i, bind(&Client::on_data, this, _1, _2, start),
                                  bind(&Client::on_nack, this, _1, _2),
                                  bind(&Client::on_timeout, this, _1, n - 1, start));
        } else {
            std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
            exit(1);
        }
    }

    void on_timeout_file(int segment, const Interest &interest, int n) {
        if (n > 0) {
            Interest i(interest);
            i.refreshNonce();
            _face.expressInterest(i, bind(&Client::on_file, this, segment, _2),
                                  bind(&Client::on_nack, this, _1, _2),
                                  bind(&Client::on_timeout_file, this, segment, _1, n - 1));
        } else {
            std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
            exit(1);
        }
    }
};

int main(int argc, char *argv[]) {
    const char *prefix = "/throughput", *file_path = "";
    int window = DEFAULT_WINDOW, start = 0;
    bool download = false;
    for (int i = 1; i < argc; i += 2) {
        switch (argv[i][1]) {
            case 'd':
                download = true;
                file_path = argv[i + 1];
                break;
            case 'w':
                if (atoi(argv[i + 1]) > 0) {
                    window = atoi(argv[i + 1]);
                }
                break;
            case 's':
                if (atoi(argv[i + 1]) > 0) {
                    start = atoi(argv[i + 1]);
                }
                break;
            case 'p':
                prefix = argv[i + 1];
                break;
            default:
            case 'h':
                std::cout << "usage: "<< argv[0] << " [options...]\n"
                          << "\t-p prefix\tthe prefix of the ndnperfserver (default = /throughput)\n"
                          << "\t-w window\tthe packet window size (default = 32)\n"
                          << "\t-d filename\tthe file to retrive, use download mode (default is benchmark mode)\n"
                          << "\t-h\t\tdisplay the help message\n" << std::endl;
                return 1;
        }
    }
    Client client(prefix, window, download, file_path);
    client.run();
    return 0;
}
