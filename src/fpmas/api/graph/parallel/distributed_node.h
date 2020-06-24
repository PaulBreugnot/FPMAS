#ifndef DISTRIBUTED_NODE_API_H
#define DISTRIBUTED_NODE_API_H

#include "fpmas/api/graph/base/node.h"
#include "fpmas/api/graph/parallel/distributed_arc.h"
#include "fpmas/api/synchro/mutex.h"
#include "distributed_id.h"

namespace fpmas::api::graph::parallel {

	template<typename T>
	class DistributedNode
		: public virtual fpmas::api::graph::base::Node<DistributedId, DistributedArc<T>> {
		typedef fpmas::api::graph::base::Node<DistributedId, DistributedArc<T>> NodeBase;
		public:
			virtual int getLocation() const = 0;
			virtual void setLocation(int) = 0;

			virtual LocationState state() const = 0;
			virtual void setState(LocationState state) = 0;

			virtual T& data() = 0;
			virtual const T& data() const = 0;
			virtual void setMutex(synchro::Mutex<T>*) = 0;
			virtual synchro::Mutex<T>& mutex() = 0;
			virtual const synchro::Mutex<T>& mutex() const = 0;

			virtual ~DistributedNode() {}
	};

}

#endif