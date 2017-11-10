# NDNperf

We designed NDNperf, an open source tool for NDN server-side performance evaluation and sizing purposes, in order to have an idea of the throughput a server can achieve when it has to generate and transmit NDN Data packets. It is very similar to iPerf and also needs a client and a server to perform the measurements while minimizing the number of instructions between Interest reception and Data emission. It exists in two flavors (<a href="https://github.com/Kanemochi/ndnperf/tree/master/java">Java</a> and <a href="https://github.com/Kanemochi/ndnperf/tree/master/c++">C++</a>) and has the following features: 

* Periodic report of performances: end-to-end throughput, latency, processing time;
* Multi-threaded (one main thread for event lookup and N threads for NDN Data generation);
* Able to use all available signatures implemented in the NDN library, choose the size of the key, and the transmission size of Data packets;
* Message broker implementation (Java version only, currently no update is sheduled).

NDNperf features many options regarding the signing process because we identified it as the main bottleneck of application performances.

You can find a study on server performance using ndnperf on a paper named "Server-side performance evaluation of NDN" and published in the ACM ICN 2016 conference.

