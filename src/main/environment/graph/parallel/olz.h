#ifndef OLZ_H
#define OLZ_H
#include "../base/graph.h"
#include "distributed_graph.h"

namespace FPMAS {
	namespace graph {
		template<class T> class DistributedGraph;
	}
}

using FPMAS::graph::DistributedGraph;
using FPMAS::graph::Node;

namespace FPMAS {
	namespace graph {
		template <class T> class GhostNode : public Node<T> {

			friend GhostNode<T>* DistributedGraph<T>::buildGhostNode(Node<T>);

			private:
				int origin_rank;
				GhostNode(Node<T>, int, const DistributedGraph<T>&);

		};

		template<class T> GhostNode<T>::GhostNode(Node<T> node, int origin_rank, const DistributedGraph<T>& local_graph)
			: Node<T>(node), origin_rank(origin_rank) {
				for(auto it = this->outgoingArcs.begin(); it != this->outgoingArcs.end();) {
					if(local_graph.getNodes().count((*it).getTargetNode()->getId()) < 1) {
						this->outgoingArcs.erase(it);
					}
					else {
						it++;
					}
				}
				for(auto it = this->incomingArcs.begin(); it != this->incomingArcs.end();) {
					if(local_graph.getNodes().count((*it).getSourceNode()->getId()) < 1) {
						this->incomingArcs.erase(it);
					}
					else{
						it++;
					}
				}
			}
	}
}

#endif
