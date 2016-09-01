#Compile the program with:

* cmake . && make

or

* g++ -o ndnperfserver server.cpp -std=c++11 -O2 -lndn-cxx -lcryptopp -lboost_system -lboost_filesystem -lboost_thread -lpthread

#How to use the program:

./ndnperfserver [options...]

####options:
* -p prefix		the prefix name to register (default = /throughput)
* -s sign_type		the type of the signature to generate 0=SHA,1=RSA,3=ECDSA (default = 1)
* -k key_size		actually limited by the lib, RSA={1024,2048} and ECDSA={256,384}
* -t thread_count		(default = CPU logical core number)
* -c chunk_size		(default = 8192)
* -f freshness		in milliseconds (default = 0)
* -x 			override the ndn sign function with ndnperf sign function
* -h 			display the help message
	
Some information about the program:
	
1. The server will answer any Interest that have the good prefix with a random data
2. If the prefix is follow by the nameComponent "download" then the server will try to find the file that correspond to the NameComponent given after "download"
