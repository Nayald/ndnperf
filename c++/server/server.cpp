//============================================================================
// Name        : server.cpp
// Author      : MARCHAL Xavier
// Version     : 1.0
// Copyright   : Your copyright notice
// Description : NDN Server in C++
//============================================================================

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
#include "blockingconcurrentqueue.h"

using namespace ndn;

#define DEFAULT_THREAD_COUNT 4
#define DEFAULT_CHUNK_SIZE 8192

const char* data8192="BpnonSeO1WTGervFuCTQA8GPaM7RM4qw5ctOAP4gr3W8p8rlIcjyoI60xoANsYx4qZb8zxLA1Lq91HxHKmoeCpmFnCKONblZG5Spb7tLAb3Fh55ODTfpQu5xXT52JoWZFFEUOHB5yUvEmXVIBeEbbJ52ySjBbEgIvo3feHmmtQMjimNH7xtRVoo7n8nsAIZLtr3jSxt7DEYG1DXYwy1EntJ7Y02yqzYX2I7RvqmiWajcDC8Dxf0JXbgZFNh9jhAThjE9DxnW5ffQZ5BEpWsBHNsJm9as1ZxAlbCOOortTK2gvsxoZM2AGtl7uJ4p5kVDpbx0FrNN2Qsf6CGcg7DBXDWFp5TSiPonGgBWvW70TxIxJggj0y5gJsUKOQJYIRIpMP3sZ7PaHpHMEb2UNQf6Fxx8z6EFRGxqiETV9vDrVIwHt7KEaqcbtYUGH4UA9Dj7zFMyFzmkWC7G3wH98QXTQzoeDXbfvQEBUiaCey0jWHkoRPE1PH4SwleOPbggjWNINNMhsG9D1L3QySXTuGMvFgBapJN5C9kWy99rIrmqDSMCFDZPGWffthLqgOvMzCep4QGSvg7ZaEfiO1F1JG4A2usVkLq81V723fIMOnToXItcSgqBtYa1vMDE2fB3yWLecTVjZTogiyXuD7hO8SJnlprsczP9yQOsQp5wBiaTTv8auMfrDKwh45jSTpTBJtOnPkRBLRuCA18PzCYmYPo13iw7OGMbIQu5vm0fBKff5jL91vEsZa9EBzM9OIlO20bD7PRl0Zt1gVC2Ba1PfDx5ovgfAD7Vk3IHqsSPATLErXJMlUT5kbtT2zIzL2xwDvgJrfBYVF9B7cayLyG4eHGSYqXjbR47eJI8zJRI5iaJQO386zmE6wpeAoBGnzZhsW8QXs6lmEb1jVMURzvTfNvPwfQxjxc1U4xX1G8q9RTONOs4CYNy9EGKm9TVvIgRUfOTllYWqZgWP4Ha4VuwcR22rZbbcxLK7c7CGh0PXOrt3fxKmwsScHTO0Ke1X30Azci3nFstf0convGoBbaVoXMxwnt1omKHE3A1TM0JrwwOkKXUHpeGFnKvnHvm9bjNKoL4FWI6y7gaNR5TnCPn09UKBHyGkV1YOhiTbK970plWRcCmwtOsfQaYa35pKGHchZPIisS1mM4kXt7NGaEGTqmVcNQ4ZFzNg719OO1FcVEocCJmUcTvyDrSFnX8hXmm0OwboYhIAm0pcfPmNjOJaA3vbSrRGhAPxo9pvIY2inTrL010NAfmRzoNfyQuPehXA7ywTSzsL5b49zqpIYKNQa3wqwfLwEFJNjMa48ftXSsBupXIBXWaEDyC8LpBMkqn5ztWaURFFXJjigkIItc0yoE4rtc1aQVqpqVy3C7XXhG1xBLvT4v8kViW6nnpjJSZC5MpE7WnXAQYDqps3nuM9VyD4DpPxVayFzDnst9QOWcQXeGv3NYVTxZv0NJU7mZC1R6NuOnyTb17pm4zGqv5sY6y9ZeFmU6VpG8LhHc8nSVEjx5kYcf2yrbzwuQH9xyQz7pzfqEzSpcsDxuaNq22I4mm7pzKRFoTEN6whQIUzpkPkmGuCnZcpsLlLKBg9SUBu70XxnpBgiRGVpB0ycR9w1giHDSrtNSHlSEuA1NL0eu2y1zKIj3S542oXKDooecoL2TuF4wZQ9hXYmwMJuORfLOzy9sDsAKYSNBWmFnaHhlz2fwCvSswqtYZ9ipHevsLHTKnjMiIGwAklCsBmL5OOmeUW9vbvPE6TbB7rxqVBANJ0Y0HgJx75ikELRpUSFINDqfS9vBKCXY219XqpfkpmO6UpmlkvRREPljs1B1ymjSnsqw608eyjDNnirij3BhnxX1LzU4s15S1bD9noKK2M3xolYu05msHCas7jXBEXuW7ORyRcW6UpW980q18xZNFIt0Hccc4oq3jKbLf5DlHyMiRJAUnpgslm4am8I1om1KUFCPmnsD3AVunxz72TjsxrgwCM7x21fQj6tmP9F1cgnYUP497DJZ2UQZVGTNP7DEnnSxFGneVtqfbARFq9C6TD4nbBKOrIBcpIwqnNWMGgRcDGwrYK0s3DB3u3ncq9FO9GGm1MGsYTkrXyryQqyscuS39gr8nn9WeNAEWpOP0rMJZW4SZ9lDQ0EhZ2Ktji2gusOFegzczpvQEbuFvwbbK9OYNkL8pD5S509WSgKL9K4BgqXC2kTTsfrw8EiSOVc5pgDJYsiRn1I59i8XzIZ9bm6tlZp4XE2Zr2LHa7hJBVYjqMfZerCnV0rIKSMZONWGj66Ro06mOUuRTjsGClFwP6W4iH15IAYwakZRRWUzMa10y9PGb6c1K421ajuM8unJg1BtjAetqUZZZ1bu1o6R8kpEVObh0o6b8GgasvYZsJQzgToREWYSaUv0GJ2K0aNwsLSlKuCShn3JRcOBphhq5uovEhkPipNmwWOhl3BH7D7P58anIjL9KpNMUUm9oiCYCqTLjk1mqDXMwhpZXr7ezhrApgxkq1u69EEOaZBzrfuGTuZPjQRtBwy9e7u4s8iXn8QCAHMUcNqx1XtwXXvaPkJC6QzIbmBw4GkKkwkOC2qjgF6SBhURTRYPaTBGVx8fJIWDxOMZOmqbSUY64k2lQIZi0FKAGToIOWWlegD5PbccWc8foYL6huJuRBPhBxnOniqtV9lywPcUoqaPQY5WINim9wLiF8U2QY4qOZOKBfJve5VVwEuLcqQ0l9fxA1TZ9YlSKb7kbr71a2vb0ZBH8J3PL5J60jVi1EzwVcqSgLvZnlvqjW9qqoCC3HFBRpfywABWxS4PGKx6tgy90O3NKcZaABXo5SLLUNyXKSbyhYwlXMYJRaRIjqx0e9GkzgxSN9N4UygHelLbB7TolNnbx2QnppYGephxhUj87Z1VJu7gnDqHDXyFkAbvh0qjwXjbIJ6nSlOGQuWWmV5mUXJHhUt8nHlY14GtqJ8y4iPrHQ0A9bL0GRXHpy9hcQB8vpuDNXezgYSVLIZIoQtxzzD7UXE6rbzMRlyvDXm26WoUDi0ytrLqa2uOnkTo15kzLzfsHLfm8ss0NAsPEE9xoPFwZr2SMH5jbGEKAU5TH6oNPl5QoLSWKUaOkRG48E1yphU8HYBtczkeLhrFMgTgazuc1o2i8Q7B47SDH7i3DGcfLugMsC7VUOwp3IyPLyryKKVC01XOfHWDVfwOvuX3MOH1WlHxlIzMt396E2zfuTRgxRpxW32gvGsntK4xnAnAKnIojA7HqnphkVFe3E9TzwMvuSGhyuU4HInKGJ65X1YVgOgaozBw1FcHYJrH0ZIptfxD0cUUOXgjhk6sBgBxNcWVym5PO894SZcU0rCmkZzyvFgWUC585MgNSZcwM6jHoTvtM05VLowaFHfGzDHB4B7zHxtoq1JQfY8Zjhb2N21KEkxkEiZyQD9zP9i9e1RcSUp2Sjk3fFtJtgzY8B0OFiMNwhA4fCm3CHgp2Wex0q3EmVHcb1Xjm0yG9IQKKfSSAEFEAiYCmFCs6KCoD6fa0pUhZ7s7h6ZfPHgUUhkPl0u5RwR7TgFjYwZznHbHWiu1POb0BE71CGDwQaL8TV001hR6Y295U4JO4MgGf0cCmZfHyHtirYIVOlRLvnIK7tsw19jhuhn1vbc3aogNGgTfA2iXwYQXq9GljANtolEsfuf5bheQ3wB87LkTwRJ8ErYT49Il0TQNJnWfiO0l8shtVm5glExilGUDJiGJ9EkV8ci1tU0Y67xIGXaXKOtOkQJxNBjPAG19LVkrr3As8Ab9Z0ekf35sSmmV8fRUb7KRDa0yjOM4mFqmfxOog1SyR6BNjk6rH0HE8Azpnz3xIalMUDFxqf4EkSE2jFrqpsco6jI8kouUsfHQTh3ihcF6eTKfqOeUi1PNVRypFkXZtQVZlinu0UJesE9vsftUmqB786lUgPVLiQlfuH8yDt2o5XqH5SGm8kw96DLqgs8hYVCMVb48m3PGVnLLCOXrEBooYikJgQKBu4C02RyZ76NzcGStNkpglDPxYU5fIUH8b8mCGjfZwHzWHLQi32Hxr1xhQtxntjgbGJa89PZySf4lRImwVy1KBrN8BlPEK8UIPhnAo21bCrwtr89KF1bqFOkRsPbHhTom1tNnqwgpSqkXbJPwfiUGMGeI80BDAG5IOP1A1U4KjcvnU9Jbp0z9DRknjJJLLHAYAxplMb6aflctG278ecXS9lFJkY8qkUwAGDJv99wF3PxEqoN9EZvNhrjiboJ4eZHlGjTE0fg2KpyPyaD7vAzbX9juQ3mKpCnDWzSgksR5KNFj93HSDs6zET6xyzPDmNT5o2484KeU7x956DlriVwHHhTkIl5W0IzexaI7vuPuRNXsGDKnRTOWklzER8D8RyDzQuCZwcbzxFMyeyEuJIURASBJLV99rQJTbjfIsGoK4tmAgueHvBkVMcqU4SAaMNrPJyRRV93QYw1TFfoenW0GXSVAWD8pw65ySR9i9Rwrz0zCiBbZNfv3NZeu2YUjRPVSFXQ4jObMEbelBYAjuTfTEvG9NwnR3DmAckLMfQeRpO17KbpJtzfKTTeESOvi1iDPzV4MtNrLpOmOTAviAFX1s3PsSDNTbjC3LuO3k7oh2cLtzljP71rfIsjQmxQcIWXY6Ky5rFVG5hZ8B4tJJXyDtumf4EUkOolRE7u1tAHYD1kVZHRMDrPVqbQWoKVAof546yBcWI753N78kpNo0D9sy4ib6xDsYa7RLCgMA3xzwiFS56sTDDYr8kHjma4DPYRoUTPGGyAotD9NEYGmRrExG7wbTafplFK2hANNP3T1yT4Z3hQmMa5Kln01yjYkCZGURoYrRIWTJIJrws8eABApqxHiX1g4tbZEZBW11niSAe3kHs0riTYtfOcZA0vAuPsuDtR1urQ9XiGTFjTgEx1bfxe2QRmrp6CsPOu70OFnNQhtMZ8FjFYQeMQ7LiCsMksZEUjJSeLUiDYC1wiwbuvQ1j3ywjkDTt9I7BbA7KiBa7pLtatVf0N2gGywxQkePkgtyCZMZxXrnEwS5eaZj7UipEHCJyQ9VAWy6fERASGJQT5sKFaFjKteeRSxSNxCmlEjx9npRrqwIx8SpnOSBlptbE00v1yY4MVVjybzQNs4y8PMKmn1MEfKrhj3Ih2UT9wpNwD61fFN6stXRoRAtJkDLgguwZOKW0FTVAMz5Jji1ESciRV0x7viZ61yh6k8czLfUHwJTcNL3bSIeO8Xxp1ux5R1sjJUww7JieYfA6UxmC9v7joDmRE0avXjzyubCVbafokE88fZ8OkaqYFt7yWcfWqZtWWQCuH6aRLy2iLBvnaYwTW4pu0LQ6BwKMh1yUrspDJYb6NjuijX3EukA7kNuyyezMF6jI0cj7uc8b76yWTAtDfQA9O9SRUvxIMNN6TvAvKBx1sT5cLpIGCCJtg328Ph18svggTVJoMRPWXCiRG6hTnVYmXNzxiNCyyY0GAZYFBrcplvimWKsHVwh3wTjcFvNyefc3hZbAtfOxNOtEbiRbJYfr2o2rrn1wlC432Zg9Fz8UBz8rpibI4q81toH4ghSk6aiF49zKrjQvEGiAhZbIXpZheTB5eLCr5nzzhcRYknB4mnOJLw1On40X1V3Br2NclSyQ6kgKApiyUCMkToG5MAhZ7KcRHcprQL22F3wIWIyWxb3HMjNMKLCrwu4ODEW8ePhg1ICRGsogiBcz05CYZ0QCJVSCTP0DDhBCPPzjJ49LQwxG80lOzt9KzDEAKjYOZyn281mwk1KDHmvIwhxJlqLCka2nNO1Inv7cyVpADH4zB7hwoIZ6FYNy4Nmh2SkkHMTDtAVTOAIT9wCv8TF4oYBIn8XXcuvDKpQiJUSYgMVRKh1hSjYz57Xw3nMm70sTcGiftwVB3CN4HLZEiPjJurYrrubJFxfhbkEycIJaNDHlFRJNCDLraROXAS18IeowXRXB6RwlwL1Dh0WC20e8CgWO28V6q9i5LettIPUNWN6r5AVseJxXluAwE8YpJa2w2FyDEOSw10saHwZ42z61mPl2GmskcckNYQN0Nn50Hb53543LNVz2ZH6QapS00IHmYhq4BKvYPMs4s3qMm2U6Kxh6CUt4fn2XOQXmGk1asl7fT4Z2P0DRxT6bLyAAo5k9FP27FvHzlcc9K8y2nukLbZrrPkC68gzICVEpfEUAGuEKPzp29loE9eC6Ka3cs83tjGn6ytXR9a3jg2cuNG6EqaCEWQOYLESoT3czNtS13HyLeY6FN6QbMJu71PL1ymzPfqjaZMpOVxYba55GtcAojy025OCB8Bb9lz9o8bluc9rziizAUnHzZCEkX70WG5utZa0Uxr5ogfTQ1KfOkHWbQhGsSELZrzpJImu8nHqOF8a95eeSEkMPXiE4lRtflLNZ0DsXysxbDw9oBKohhTMz3kXHe7pSLFsZHZazOsmlOkcVqoWI8mtAfzVGbhufwWgWHiGM432biZUFSlmr4TlWRFPrirGn9IJDRTstXJSzLSZjDoo0kg4IOQnYN6x0vvKBfzifJRal9VxQTnwalf4jXnmEpr0DrswPwyopjJGNfDqqH5u5Spe0TcB5xOzWTONtBZPeEU4fJf0bGrbEk7VSPR4sYHwtaGoUFy8kkHMPubaLWGLjaQkMUDpXMFxurVR5jjcqmYXL1NnAa6YRVxQMzXqAmLw6vsz0UQqflaTzgbcP6RNK6uVDl7b6Q0EJVEPArG4PHBsLsv3j61hnym6HL76fMgMH98qGjRWSq1UIzl5N8ZBQ1i9YERmgShV7aKlnyPgyJm1IHrnAS3zUZBLvvzyBg256eHKBkDKllKipMDqa9g24Gn6OgoGYKhs5Q7gPwb8mESslF2QlyuW5px1Ucphwvmyw9KkcJ1FIquoRmxipmPgA59ehiILkWhVHpU2Rs45lC85hqlFLG9r08QIbwSibN1Z2HLaovPIOtiY4Q4ADT00PrAKnFxrAx9Zm2IeHFNRFPyqKYU43gDqVHKCLioyQXAsuEe44IYuLIcztN1wfNo9i0g6DSk6M2DDluGErvS06e4nhmhO0yk5WEcTSpNlk9pvTXJUVtAyGBtRsB7VBR0356ymGXyevkjDE1H5IiagMzvJalRtH7FLBs4yhKtDLUTHgth71OiVnRE7fu4JFCvTDTMv1lNIpvVUtnyurf2WNeuCnwiS7GjoBueIP3uAjXFYbWe3tAcoRiNCpjTmF8hakmpXCNUhXTu1Xw4smkuqS7LNZ4qCUWbSrQIeeTLQH9E3xSFq4ofWfHBZvBU4L2E47c9ZunbR7RotQHkDJgks2Ii91MCiUBMy0kWQzsg2WofR4spE0sac8OBHPlZtEk1UlMa4wex8TNB95jG73hjCgLp3XQQ4RbcwnfRO7B49ZtxT6ubltBOQvbeq1WKQjjZt7Ubh0AzL3WPOyK5SDbmI4lYOtSNY53n6hr0cTFGVp3oFeeUFh3aTb84oJayAxwztjTEIkBOSQlGlEo67hbCclFBZ3I8n699hV8OiknYfWAE6PmFKErEjFAYUeXPBKEjVRmVYBmTuwLKh0qaKWLJzL5GWJqAil4xYBQCZSOxAk6gWUQY6ID4cBYfH6q8GvnrBJ9srpCDT4yQa6QHJqFTIGXD7B55VGSOBhhVjQgHoKhraDTV0kF3pAhIjYFn1hl3opkOKlUa3VBa0WifvWJ1qCv3vDRZ0hZGokfwDP7NwH9QmZPlVxHVLNIZt1MStY1gGxVZfIjh1vboBHWCpTRYK0LRZcLa92arO9xCjxReAufJ7xXRVqR4BzSzEJHC1KFkSG0vhfmknVYPYpxa5qSFoOwpTOzgntn6XRV1QlTGqZi3ATze4D8X58t0jz6mix9QGVvwGE0zxHMFRz7MS5uc6a9IcJQFn8DD1VYkuZtgsefICBKmO6noIgg4nwlMJKXSyv52cnyfijD2ZO29eB5487zB7FIT3u2ZDMYZygyC737EpCntrG5ABR2RCSlGeYY98v83OarK4VyeyBTU3nMjE98LnoRyRmCqhyQAOzPajTYZM1TkqLpoXIs26Qt4oyw8EvBIcPgMw1zLCQjHiffA2Yx5HqmEbL9tIG3RYGxeEcJgEJqlT2Zpx5XWzawr1ibigeXtaKBxI5vExyXs65zsr4HDaFFBPvffoAxZZmn63m8to1qKZ2V8UnHHOOyJw6J8wb3bYF0NJc0xjysvsxjt3sJVF3rwkL4wOlzDy";

class Server{
private:
	Face face;
	const char* prefix;
	KeyChain keyChain;
	moodycamel::BlockingConcurrentQueue<Interest> queue;
	int k;
	std::thread *t;
	std::atomic<int> count;
	Block content;
	int content_size;

public:
	Server(const char* prefix, int thread_count, int chunk_size):prefix(prefix),k(thread_count),count(0){
		t=new std::thread[k+1];
		shared_ptr<Buffer> buf = make_shared<Buffer>(data8192,chunk_size);
		content=Block(tlv::Content,buf);
		content_size=content.value_size();
		std::cout << "chunk_size = " << content_size << " Bytes" << std::endl;
	}

	~Server() {
		for(int i=0;i<k;++i)
			t[i].join();
		delete[] t;
	}

	void start() {
		face.setInterestFilter(prefix,bind(&Server::on_interest, this, _2),
				bind(&Server::on_register_failed, this));

		for(int i=0;i<k;++i)
			t[i]=std::thread(&Server::process,this);
		std::cout<< "init " << k << " threads!" << std::endl;
		t[k]=std::thread(&Server::display,this);

		face.processEvents();
	}

	void process() {
		Interest i;
		shared_ptr<Data> data;
		while(true){
			queue.wait_dequeue(i);
			//std::cout << "interest receive !" << std::endl;
			//std::cout << i.getName() << std::endl;
			data = make_shared<Data>(i.getName());
			data->setFreshnessPeriod(time::milliseconds(0));
			data->setContent(content);
			keyChain.sign(*data);
			face.put(*data);
			count+=content_size;
		}
	}

	void display(){
		std::time_t time;
    		char mbstr[28];
    		while(true){
			std::this_thread::sleep_for(std::chrono::seconds(4));
			time=std::time(NULL);
			std::strftime(mbstr, sizeof(mbstr), "%c - ", std::localtime(&time));
			std::cout << mbstr << (count>>12) << " KB/s" << std::endl;
			count=0;
		}
	}

	void on_interest(const Interest& interest) {
		queue.enqueue(interest);
	}

	void on_register_failed()
	{
		exit(1);
	}
};

int main(int argc, char *argv[]) {
	int thread_count=DEFAULT_THREAD_COUNT,chunk_size=DEFAULT_CHUNK_SIZE;
	const char *prefix="/debit";
	for(int i=1;i<argc;i+=2){
		switch(argv[i][1]){
		case 't':
			thread_count=atoi(argv[i+1])>0?atoi(argv[i+1]):DEFAULT_THREAD_COUNT;
			break;
		case 'c':
			chunk_size=atoi(argv[i+1])>0?atoi(argv[i+1]):DEFAULT_CHUNK_SIZE;
			break;
		case 'h':
		default:
			std::cout << "usage: ./ndnserver [-t thread_count] [-c chunk_size]" << std::endl;
			return 0;
		}
	}
	Server server(prefix,thread_count,chunk_size);
	server.start();
	return 0;
}

