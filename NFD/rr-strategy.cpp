#include "rr-strategy.hpp"

namespace nfd {
	namespace fw {
		
		const Name RRStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/round-robin/%FD%01");
		NFD_REGISTER_STRATEGY(RRStrategy);

		RRStrategy::RRStrategy(Forwarder& forwarder, const Name& name):Strategy(forwarder, name){
			i=0;
		}

		RRStrategy::~RRStrategy(){}

		void RRStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest, shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry){
			const fib::NextHopList& nexthops = fibEntry->getNextHops();
			fib::NextHopList::const_iterator it = nexthops.begin();
			size_t size=nexthops.size();
			if(i>=size)i=0;
			unsigned int j=0;
			while(++j<=i)++it;
			++i;
			shared_ptr<Face> outFace = it->getFace();
			if (pitEntry->canForwardTo(*outFace))this->sendInterest(pitEntry, outFace);
		}
	} // namespace fw
} // namespace nfd
