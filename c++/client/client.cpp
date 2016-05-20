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
//#include <ndn-cxx/security/validator-regex.hpp>

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

#define DEFAULT_WINDOW 32

class Client{
private:
	Face face;
	Name prefix;
	const int window;
	int current_pkt;
	bool first;
	std::atomic<int> count, sum_size, sum_rtt;
	std::thread *t;
	//ValidatorRegex validator;

	bool download;
	int max_segment,current_segment;
	std::map<int,Data> pending_segments;
	std::ofstream file;
	std::string file_name;

public:
	Client(const char* prefix, int s, int n, bool download, const char* file_name)
	: prefix(prefix), current_pkt(s), window(n), first(true), count(0)
	, sum_size(0), sum_rtt(0), download(download), file_name(file_name)
	, max_segment(0), current_segment(0){}
	~Client(){}

	void run() {
		std::cout << "Client start with window = " << window << std::endl;
		if(download){
			file.open(file_name);
			t = new std::thread(&Client::display_file,this);
		}else{
			t = new std::thread(&Client::display,this);
		}
		int max=current_pkt+window;
		for(current_pkt; current_pkt<max;++current_pkt){
			Name name = Name(prefix);
			if(download)name.append("download").append(file_name);
			else name.append("benchmark");
			name.append(std::to_string(current_pkt));
			Interest interest= Interest(name,time::milliseconds(4000));
			interest.setMustBeFresh(true);
			if(download){
				face.expressInterest(interest, bind(&Client::on_file, this, current_pkt, _2)
					, bind(&Client::on_nack, this, _1, _2)
					, bind(&Client::on_timeout_file, this, current_pkt, _1, 5));
			}else{
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				face.expressInterest(interest, bind(&Client::on_data, this, _1, _2, start)
					, bind(&Client::on_nack, this, _1, _2)
					, bind(&Client::on_timeout, this, _1, 5, start));
			}
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

	void display_file(){
		std::time_t time;
		char mbstr[28];
		int sum=0;
		while(true){
			std::this_thread::sleep_for(std::chrono::seconds(2));
			time=std::time(NULL);
			std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
			std::cout << mbstr << current_segment-1 << "/" << max_segment << " completed - "<< (sum_size.exchange(0)>>8) << " Kbps" << std::endl;
        	}
	}

	/*void on_data2(const Interest& interest, const Data& data){
		validator.validate(data,
                       bind(&Client::on_data, this, _1),
                       bind(&Client::on_fail, this, _2));
	}*/

	void on_data(const Interest& interest, const Data& data, std::chrono::steady_clock::time_point start) {
		sum_rtt.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count());
		if(first){
			std::cout << "Server signature type = " << data.getSignature().getType() << std::endl;
			std::cout << "Server packet size = " << data.getContent().value_size() << std::endl;
			first=false;
		}
		Interest i = Interest(Name(prefix).append("benchmark").append(std::to_string(current_pkt++)),time::milliseconds(4000)).setMustBeFresh(true);
		start = std::chrono::steady_clock::now();
		face.expressInterest(i, bind(&Client::on_data, this, _1, _2, start), bind(&Client::on_nack, this, _1, _2), bind(&Client::on_timeout, this, _1, 5, start));
		count.fetch_add(1);
		sum_size.fetch_add(data.getContent().value_size());
	}

	void on_file(int segment, const Data& data){
		//std::cout << data << std::endl;
		max_segment = stoi(data.getFinalBlockId().toUri());
		sum_size.fetch_add(data.getContent().value_size());
		pending_segments[segment]=data;
	    	while(pending_segments.find(current_segment)!=pending_segments.end()){
			file.write(reinterpret_cast<const char*>(pending_segments.at(current_segment).getContent().value()),pending_segments.at(current_segment).getContent().value_size());
			pending_segments.erase(current_segment);
			++current_segment;
		}
		if(current_segment > max_segment){
			file.close();
		}
		if(current_pkt <= max_segment){
			Interest i = Interest(Name(prefix).append("download").append(file_name).append(std::to_string(current_pkt)),time::milliseconds(4000)).setMustBeFresh(true);
			face.expressInterest(i, bind(&Client::on_file, this, current_pkt, _2), bind(&Client::on_timeout_file, this, current_pkt, _1, 5));
			++current_pkt;
		}
	}

	/*void on_fail(const std::string& reason){
		throw std::runtime_error(reason);
	}*/

	void on_nack(const Interest& interest, const lp::Nack nack){
		std::cout << "Nack receive : " << nack.getReason() << std::endl;
		exit(-1);
	}

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

	void on_timeout_file(int segment, const Interest& interest, int n) {
		if (n > 0) {
			Interest i(interest);
			i.refreshNonce();
			face.expressInterest(interest, bind(&Client::on_file, this, segment, _2), bind(&Client::on_timeout_file, this, segment, _1, n-1));
		} else {
			std::cout << "Timeout for interest " << interest.getName().toUri() << std::endl;
			exit(1);
		}
	}
};

int main(int argc, char *argv[]) {
	const char *prefix="/throughput", *file_name="";
	int window = DEFAULT_WINDOW, start=0;
	bool download=false;
	for(int i=1;i<argc;i+=2){
		switch(argv[i][1]){
			case 'd':
				download=true;
				file_name = argv[i+1];
				break;
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
			case 'h':
				std::cout << "usage: ./ndnperf [options...]\n"
				<< "\t-p prefix\tthe prefix of the ndnperfserver (default = /throughput)\n"
				<< "\t-w window\tthe packet window size (default = 32)\n"
				<< "\t-s startfrom\tthe starting value for the final nameComponent (default = 0)\n"
				<< "\t-d filename\tspecify the file to retrieve (default = 0)\n"
				<< "\t-h\t\tdisplay the help message\n" << std::endl;
				break;
		}
	}
	Client client(prefix,start,window,download,file_name);
	client.run();
	return 0;
}
