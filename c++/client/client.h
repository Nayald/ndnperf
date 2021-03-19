#pragma once

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/transport/tcp-transport.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/lp/nack.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <string>
#include <chrono>

#include "module.h"

class Client : public Module {
private:
    ndn::Name _prefix;
    ndn::Face _face;
    bool _exit_on_nack = false;

    size_t _window;
    bool _first = true;
    size_t _pkt_count = 0;
    size_t _payload_size = 0;
    size_t _rtt = 0;
    boost::asio::deadline_timer _timer;

    bool download = false;
    size_t _current_packet = 0;
    size_t _current_segment = 0;
    size_t _max_segment = 0;
    std::unordered_map<size_t, ndn::Data> _pending_segments;
    std::ofstream _file;
    std::string _file_path;

public:
    Client(const std::string &prefix, size_t window, const std::string &file_path);

    ~Client() override = default;

    void run() override;

    void waitNextDisplay();

    void onNack(const ndn::Interest &interest, const ndn::lp::Nack &nack);

    void onData(const ndn::Interest &interest, const ndn::Data &data, std::chrono::steady_clock::time_point start);

    void onTimeout(const ndn::Interest &interest, int n, std::chrono::steady_clock::time_point start);

    void display();

    void onDataFile(size_t segment, const ndn::Data &data);

    void onTimeoutFile(int segment, const ndn::Interest &interest, int n);

    void displayFile();

    void setExitOnNack(bool state);
};
