#ifndef NFD_DAEMON_FW_ROUND_ROBIN_STRATEGY_HPP
#define NFD_DAEMON_FW_ROUND_ROBIN_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
	namespace fw {
		class RRStrategy : public Strategy{
			public:
  				RRStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  			virtual ~RRStrategy();

  			virtual void afterReceiveInterest(const Face& inFace, const Interest& interest, shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;

			public:
  				static const Name STRATEGY_NAME;

			private:
				unsigned int i;
		};
	} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ROUND_ROBIN_STRATEGY_HPP
