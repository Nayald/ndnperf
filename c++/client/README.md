# Compile the program with:

* cmake . && make

or

* g++ -o ndnperf client.cpp main.cpp -std=c++14 -O2 -lndn-cxx -lboost_system -lboost_thread -lboost_program_options -lpthread

# How to use the program:

./ndnperf [options...]

## options:
* -p prefix&nbsp;&nbsp;&nbsp;&nbsp;the prefix name to register (default = /throughput)
* -a address&nbsp;&nbsp;&nbsp;&nbsp;use TCP face with address (uses FaceUri so ip or domain name should work, format is protocol://ip:port)
* -w window&nbsp;&nbsp;&nbsp;&nbsp;the packet window size (default = 32)
* -d filename&nbsp;&nbsp;&nbsp;&nbsp;the file to retrive, use download mode (default is benchmark mode)
* --eon&nbsp;&nbsp;&nbsp;&nbsp;exit on received nack
* -h&nbsp;&nbsp;&nbsp;&nbsp;display the help message
