Compile the program with:

g++ -o ndnperf client.cpp -std=c++11 -O2 -lndn-cxx -lboost_system -lpthread

How to use the program:

./ndnperfserver [options...]

options:
	-p prefix		the prefix name to register (default = /throughput)
	-w window		the packet window size (default = 32)
	-s startfrom		the starting value for the final nameComponent (default = 0)
	-d filename		the file to retrive, use download mode (default is benchmark mode)
	-h			display the help message
