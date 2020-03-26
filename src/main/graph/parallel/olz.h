#ifndef OLZ_H
#define OLZ_H

#include "../base/graph.h"
#include "synchro/ghost_mode.h"

using FPMAS::communication::SyncMpiCommunicator;
using FPMAS::graph::parallel::proxy::Proxy;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	using base::LayerId;

	namespace synchro::wrappers {
		template <typename T, int N> class GhostData;
		template <typename T, int N, SYNC_MODE> class SyncData;
	}

	using synchro::wrappers::SyncData;
	using synchro::wrappers::GhostData;

	template<typename T, int N, SYNC_MODE> class GhostGraph;

	/**
	 * GhostNode s are used to locally represent nodes that are living on
	 * an other processor, to maintain data continuity and the global graph
	 * structure.
	 *
	 * They are designed to be used as normal nodes in the graph structure.
	 * For example, when looking at nodes linked to a particular node, no
	 * difference can be made between nodes really living locally and ghost
	 * nodes.
	 */
	template <typename T, int N, SYNC_MODE> class GhostNode : public Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N> {
		friend SyncData<T, N, S>;
		friend GhostNode<T, N, S>* GhostGraph<T, N, S>
			::buildNode(DistributedId);
		friend GhostNode<T, N, S>* GhostGraph<T, N, S>
			::buildNode(Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>&, std::set<DistributedId>);

		private:
		GhostNode(SyncMpiCommunicator&, Proxy&, DistributedId);
		GhostNode(SyncMpiCommunicator&, Proxy&, Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>&);
	};

	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>
		::GhostNode(SyncMpiCommunicator& mpiComm, Proxy& proxy, DistributedId id)
		: Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(
				id,
				1.,
				std::unique_ptr<SyncData<T,N,S>>(
					S<T,N>::wrap(id, mpiComm, proxy
					)
				)	
			) {}

	/**
	 * Builds a GhostNode as a copy of the specified node.
	 *
	 * @param node original node
	 */
	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>
		::GhostNode(
			SyncMpiCommunicator& mpiComm,
			Proxy& proxy,
			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>& node
			)
		: Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(
			node.getId(),
			node.getWeight(),
			std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
				node.getId(), mpiComm, proxy, std::move(node.data()->get())
				)
			)
		) {}

	/**
	 * GhostArcs should be used to link two nodes when at least one of the
	 * two nodes is a GhostNode.
	 *
	 * They behaves exactly as normal arcs. In consequence, when looking at
	 * a node outgoing arcs for example, no difference can directly be made
	 * between local arcs and ghost arcs.
	 *
	 */
	template<typename T, int N, SYNC_MODE> class GhostArc : public Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N> {
		friend GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
				GhostNode<T, N, S>*,
				Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>*,
				DistributedId,
				LayerId);
		friend GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
				Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>*,
				GhostNode<T, N, S>*,
				DistributedId,
				LayerId);
		friend GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
				GhostNode<T, N, S>*,
				GhostNode<T, N, S>*,
				DistributedId,
				LayerId);

		private:
		GhostArc(
			DistributedId,
			GhostNode<T, N, S>*,
			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>*,
			LayerId);
		GhostArc(
			DistributedId,
			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>*,
			GhostNode<T, N, S>*,
			LayerId);
		GhostArc(
			DistributedId,
			GhostNode<T, N, S>*,
			GhostNode<T, N, S>*,
			LayerId);

	};

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular incoming arcs list of the
	 * local target node.
	 *
	 * @param arcId arc id
	 * @param source pointer to the source ghost node
	 * @param target pointer to the local target node
	 */
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>::GhostArc(
			DistributedId arcId,
			GhostNode<T, N, S>* source,
			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* target,
			LayerId layer
			)
		: Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(arcId, source, target, layer) { };

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular outgoing arcs list of the
	 * local source node.
	 *
	 * @param arcId arc id
	 * @param source pointer to the local source node
	 * @param target pointer to the target ghost node
	 */
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>::GhostArc(
			DistributedId arcId,
			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* source,
			GhostNode<T, N, S>* target,
			LayerId layer
			)
		: Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(arcId, source, target, layer) { };

	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>::GhostArc(
			DistributedId arcId,
			GhostNode<T, N, S>* source,
			GhostNode<T, N, S>* target,
			LayerId layer
			)
		: Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(arcId, source, target, layer) { };

}

#endif
