#include <ndn-cxx/data.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <string.h>
#include <thread>
#include <chrono>
#include <utility>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>
//#include <ndn-cxx/security/signing-helpers.hpp>
//#include <ndn-cxx/security/signing-info.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <thread>
#include <map>


#include <mpi.h>

using namespace std;

class sample{
private:
        const char *a,*b,*c;
public:
        sample(const char* a,const char* b,const char* c):a(a),b(b),c(c){}
        const char* getA(){
                return a;
        }
        const char* getB(){
                return b;
        }
        const char* getC(){
                return c;
        }
};

unsigned char* serialize(sample s){
        unsigned char* tmp=new unsigned char[100];
        int pos=0;
        int i=strlen(s.getA())+1;
        memcpy(tmp+pos,&i,sizeof(int));
        pos+=sizeof(int);
        memcpy(tmp+pos,s.getA(),i);
        pos+=i;
        int j=strlen(s.getB())+1;
        memcpy(tmp+pos,&j,sizeof(int));
        pos+=sizeof(int);
        memcpy(tmp+pos,s.getB(),j);
        pos+=j;
        int k=strlen(s.getC())+1;
        memcpy(tmp+pos,&k,sizeof(int));
        pos+=sizeof(int);
        memcpy(tmp+pos,s.getC(),k);
        pos+=k;
        unsigned char *raw=new unsigned char[pos];
        memcpy(raw,tmp,pos);
        for(int i=0;i<pos;i++)printf("%02x ",raw[i]);
        cout << endl;
        return raw;
}

sample deserialize(const unsigned char* raw){
        int pos=0;
        int i=(int)raw[pos];
        pos+=sizeof(int);
        char *a=new char[i];
        memcpy(a,raw+pos,i);
        pos+=i;
        int j=(int)raw[pos];
        pos+=sizeof(int);
        char *b=new char[j];
        memcpy(b,raw+pos,j);
        pos+=j;
        int k=(int)raw[pos];
        pos+=sizeof(int);
        char *c=new char[k];
        memcpy(c,raw+pos,k);
        return sample(a,b,c);
}

unsigned char* seri_data(const ndn::Data d){
	unsigned char* tmp=new unsigned char[9000];
	int pos=0;

	string s=d.getName().toUri();
	int i=s.length()+1;
	memcpy(tmp+pos,&i,sizeof(int));
	pos+=sizeof(int);
	memcpy(tmp+pos,s.c_str(),i);
	pos+=i;

	ndn::time::milliseconds t=d.getFreshnessPeriod();
	memcpy(tmp+pos,&t,sizeof(ndn::time::milliseconds));
	pos+=sizeof(ndn::time::milliseconds);

	const unsigned char* c=d.getContent().value();
	int j=d.getContent().value_size();
	memcpy(tmp+pos,&j,sizeof(int));
	pos+=sizeof(int);
	memcpy(tmp+pos,c,j);
	pos+=j;

	const unsigned char* sss=d.getSignature().getValue().value();
	int k=d.getSignature().getValue().value_size()+1;
	memcpy(tmp+pos,&k,sizeof(int));
	pos+=sizeof(int);
	memcpy(tmp+pos,sss,k);
	pos+=k;

	unsigned char *raw=new unsigned char[pos];
        memcpy(raw,tmp,pos);
        for(int i=0;i<pos;i++)printf("%02x ",raw[i]);
        cout << endl;
        return raw;
}

ndn::Data dese_data(const unsigned char* raw){
	int pos=0;

	int i=(int)raw[pos];
	pos+=sizeof(int);
        char *a=new char[i];
        memcpy(a,raw+pos,i);
        pos+=i;
        string name(a);

	ndn::time::milliseconds t=(ndn::time::milliseconds)raw[pos];
        pos+=sizeof(ndn::time::milliseconds);

        int k=(int)raw[pos];
        pos+=sizeof(int);
        char *c=new char[k];
        memcpy(c,raw+pos,k);
	string content(c);

	int l=(int)raw[pos];
	pos+=sizeof(int);
	unsigned char* sss=new unsigned char[l];
	memcpy(sss,raw+pos,l);
	pos+=l;

	ndn::Data d(name);
	d.setFreshnessPeriod(t);
	d.setContent(reinterpret_cast<const uint8_t*>(content.c_str()),content.length());
        //d.setSignature(ndn::Signature(ndn::Block(reinterpret_cast<const uint8_t*>(sss),l-1)));
	return d;
}

int main(int argc, char **argv){
	int rank, size;
	int message;

	MPI::Init();
	rank = MPI::COMM_WORLD.Get_rank();
	size = MPI::COMM_WORLD.Get_size();
	if(rank==0){
		bool ok=false;
		bool probe;
		unsigned char *c;
		do{
			probe=MPI::COMM_WORLD.Iprobe(1,0);
			if(probe){
				MPI::COMM_WORLD.Recv(&message, 1, MPI::INT, 1, 0);
				c=new unsigned char[message];
				MPI::COMM_WORLD.Recv(c, message, MPI::CHAR, 1, 0);
				for(int i=0;i<message;i++)printf("%02x ",c[i]);
        			cout << endl;
				//sample ss=deserialize(c);
				//std::cout<< "message: "<< ss.getA() << " " << ss.getB() << " " << ss.getC() << std::endl;
				ndn::Data data=dese_data(c);
				std::cout << "data name: " << data.getName().toUri() << "\ndata freshness: " << data.getFreshnessPeriod() << "\ndata content: " << data.getContent().value() << std::endl;
				ok=true;
			}else{
				std::cout << "no message" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}while(!ok);
	}else{
		std::this_thread::sleep_for(std::chrono::seconds(2));
		char *a="test",*b="coucou",*c="hello111";
		//sample s(a,b,c);
		//const unsigned char *d=serialize(s);
		//message=33;
		string name("/debit");
		ndn::Data data(name);
		data.setFreshnessPeriod(ndn::time::milliseconds(15));
		data.setContent(reinterpret_cast<const uint8_t*>("coucou tout le monde"),21);
		ndn::KeyChain k;
		//k.sign(data);
		message=306;
		const unsigned char* d=seri_data(data);
		MPI::COMM_WORLD.Send(&message, 1, MPI::INT, 0, 0);
		MPI::COMM_WORLD.Send(d, message, MPI::CHAR, 0, 0);
	}
	MPI::Finalize();
	return 0;
}
