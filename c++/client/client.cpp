//============================================================================
// Name        : Client.cpp
// Author      : MARCHAL Xavier
// Version     :
// Copyright   : Your copyright notice
// Description : NDN client in C++, Ansi-style
//============================================================================

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
//#include <ndn-cxx/security/validator-null.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

using namespace ndn;

#define DEFAULT_WINDOW 32

class Client{
private:
	Face face;
	const char* prefix;
	const int window;
	int current_pkt;
	bool first;
	std::atomic<int> count, sum_size, sum_rtt;
	//ValidatorNull validator;

public:
	Client(const char* prefix, int s, int n):prefix(prefix), current_pkt(s), window(n), first(true), count(0), sum_size(0), sum_rtt(0){}
	~Client(){}

	void run() {
		std::cout << "Client start with window = " << window << std::endl;
		std::thread t(&Client::display,this);
		int max=current_pkt+window;
		for(current_pkt; current_pkt<max;current_pkt++){
			Name name = Name(prefix);
			name.append(std::to_string(current_pkt));
			Interest interest= Interest(name,time::milliseconds(4000));
			interest.setMustBeFresh(true);
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			face.expressInterest(interest, bind(&Client::on_data, this, _1, _2, start), bind(&Client::on_timeout, this, _1, 5, start));
		}
		face.processEvents();
	}

	void display(){
		std::time_t time;
		char mbstr[28];
		int sum=0,cpt=0,rtt=0;
		while(true){
			std::this_thread::sleep_for(std::chrono::seconds(2));
			sum=sum_size.exchange(0);
			cpt=count.exchange(0);
			rtt=sum_rtt.exchange(0);
			rtt/=cpt!=0?cpt:-1;
			sum>>=8; // in kilobits per sec (10 + 1 - 3), in decimal 8/(1024*2);
			cpt>>=1; // per sec;
			time=std::time(NULL);
			std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
			std::cout << mbstr << sum << " Kbps ( " << cpt << " pkt/s) - latency = " << rtt << " us" << std::endl;
		}
	}


	//void on_data2(const Interest& interest, const Data& data){
	//	validator.validate(data,
        //               bind(&Client::on_data, this, _1),
        //               bind(&Client::on_fail, this, _2));
	//}

	void on_data(const Interest& interest, const Data& data, std::chrono::steady_clock::time_point start) {
		sum_rtt.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count());
		if(first){
			std::cout << "Server signature type = " << data.getSignature().getType() << std::endl;
			first=false;
		}
		Interest i = Interest(Name(prefix).append(std::to_string(current_pkt++)),time::milliseconds(4000)).setMustBeFresh(true);
		start = std::chrono::steady_clock::now();
		face.expressInterest(i, bind(&Client::on_data, this, _1, _2, start), bind(&Client::on_timeout, this, _1, 5, start));
		count.fetch_add(1);
		sum_size.fetch_add(data.getContent().value_size());
	}

	//void on_fail(const std::string& reason){
	//	throw std::runtime_error(reason);
	//}

	void on_timeout(const Interest& interest, int n, std::chrono::steady_clock::time_point start) {
		if (n > 0) {
			Interest i(interest);
			i.refreshNonce();
			face.expressInterest(interest, bind(&Client::on_data, this, _1, _2, start), bind(&Client::on_timeout, this, _1, n-1, start));
		} else {
			std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
			exit(1);
		}
	}
};

int main(int argc, char *argv[]) {
	const char* prefix="/throughput";
	int window = DEFAULT_WINDOW,
		start=0;
	for(int i=1;i<argc;i+=2){
		switch(argv[i][1]){
			case 'w':
				window = atoi(argv[i+1]) > 0 ? atoi(argv[i+1]) : DEFAULT_WINDOW;
				break;
			case 's':
				start = atoi(argv[i+1]) > 0 ? atoi(argv[i+1]) : 0;
				break;
			case 'p':
				prefix = argv[i+1];
				break;
			default:
				break;
		}
	}
	Client client(prefix,start,window);
	client.run();
	return 0;
}
