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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>

using namespace ndn;

#define DEFAULT_WINDOW 32

class Client{
private:
	volatile bool continuer;
	int window;
	Face face;
	Interest interest;
	std::map<const std::string, int> retry_table;
	std::atomic<int> count;
//std::chrono::time_point<std::chrono::system_clock> stopTime;
//std::chrono::time_point<std::chrono::system_clock> startTime;

public:
	Client(int n) :
		continuer(true), window(n), face(), retry_table(), count(0){
		Name name = Name("");
		Interest interest= Interest(name,time::milliseconds(4000));
		interest.setMustBeFresh(true);	
	}

	~Client(){}

	void run() {
		std::cout << "Client start !" << std::endl;
		for(int i=0; i<window;i++){
			Name name = Name("/debit2/benchmark");
			name.append(std::to_string(i));
			Interest interest= Interest(name,time::milliseconds(4000));
			interest.setMustBeFresh(true);
			face.expressInterest(interest,
						bind(&Client::on_data, this),
						bind(&Client::on_timeout, this));
		}
		face.processEvents();	
	}

	void on_data() {
		//std::chrono::duration<double> duration = std::chrono::system_clock::now() - startTime;
		//std::cout << duration.count() << std::endl;
		//startTime=std::chrono::system_clock::now();
		Name name = Name("/debit2/benchmark/"+std::to_string(window++));		
		Interest i= Interest(interest);
		i.setName(name);
		face.expressInterest(i,bind(&Client::on_data,this),
				bind(&Client::on_timeout,this));
		//count+=data.getContent().value_size();
	}

	void on_timeout() {
		/*if (retry_table.find(interest.getName().toUri()) == retry_table.end())
			retry_table[interest.getName().toUri()] = 2;
		else {
			if (retry_table[interest.getName().toUri()] > 0) {
				face.expressInterest(interest,
						bind(&Client::on_data, this, _1, _2),
						bind(&Client::on_timeout, this, _1));
				retry_table[interest.getName().toUri()]--;
			} else {
				cout << "Timeout after 3 retries" << endl;
				continuer = false;
				exit(1);
			}
		}*/
		std::cout << "tm" << std::endl;
		exit(1);
	}
};

int main(int argc, char *argv[]) {
	if (argc > 1) {
			int value = argc > 1 ? atoi(argv[1]) : 0;
			if (value <= 0)
				value = DEFAULT_WINDOW;
			Client client(value);
			client.run();
		}
	return 0;
}

