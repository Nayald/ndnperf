# Compile the program with:

* cmake . && make

or

* g++ -o ndnperfserver server.cpp -std=c++14 -O2 -lndn-cxx -lboost_system -lboost_filesystem -lboost_thread -lpthread

# How to use the program:

./ndnperfserver [options...]

## options:
* -k key_type&nbsp;&nbsp;&nbsp;&nbsp;key_type={sha, rsa, ecdsa} (default = sha)
* -s key_size&nbsp;&nbsp;&nbsp;&nbsp;size of the key, rsa >= 1024, ecdsa={256, 384}
* -t thread_count&nbsp;&nbsp;&nbsp;&nbsp;number of CPU used to handle Interests (default = logical core count)
* -c payload_size&nbsp;&nbsp;&nbsp;&nbsp;size of the data carried by the packet (default = 8192)
* -f freshness&nbsp;&nbsp;&nbsp;&nbsp;freshness of the Data in milliseconds (default = 0)
* -h&nbsp;&nbsp;&nbsp;&nbsp;display this help message
	
### Some information about the program:
	
1. The server will answer any Interest that have the good prefix with a random data
2. If the prefix is follow by the nameComponent "download" then the server will try to find the file that correspond to the NameComponent given after "download" (relative path)
