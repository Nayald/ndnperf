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

#include <boost/program_options.hpp>

#include <iostream>
#include <csignal>

#include "client.h"

static const int DEFAULT_WINDOW = 32;

static bool stop = false;

static void signal_handler(int signum) {
    stop = true;
}

int main(int argc, char *argv[]) {
    std::string prefix;
    std::string file_path;
    size_t window;
    size_t start = 0;

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
            (",p", po::value<std::string>(&prefix)->default_value("/throughput"), "the prefix of the ndnperfserver")
            (",w", po::value<size_t>(&window)->default_value(DEFAULT_WINDOW), "the packet window size")
            (",d", po::value<std::string>(&file_path)->default_value(""), "the file to retrieve (unspecified = benchmark)")
            ("eon", po::bool_switch()->default_value(false), "exit on received nack")
            ("help,h", "display the help message");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm); // can throw

        // --help option
        if (vm.count("help")) {
            std::cout << "NDNperf Client" << std::endl
                      << desc << std::endl;
            return 0;
        }

        po::notify(vm); // throws on error, so do after help in case there are any problems
    } catch(const po::error &e) {
        std::cerr << e.what() << std::endl
                  << desc << std::endl;
        return -1;
    }

    Client client(prefix, window, file_path);
    client.setExitOnNack(vm["eon"].as<bool>());

    signal(SIGINT, signal_handler);

    client.start();

    do {
        sleep(15);
    } while (!stop);

    client.stop();

    return 0;
}
