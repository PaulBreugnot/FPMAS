#ifndef DISTRIBUTED_NODE_API_H
#define DISTRIBUTED_NODE_API_H

#include "api/graph/base/node.h"
#include "api/graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename T>
	class DistributedNode 
		: public virtual FPMAS::api::graph::base::Node<T, DistributedId, DistributedArc<T>> {
		typedef FPMAS::api::graph::base::Node<T, DistributedId, DistributedArc<T>> node_base;
		public:
			using typename node_base::arc_type;
			virtual int getLocation() const = 0;
			virtual void setLocation(int) = 0;

			virtual LocationState state() const = 0;
	};

}
#endif
