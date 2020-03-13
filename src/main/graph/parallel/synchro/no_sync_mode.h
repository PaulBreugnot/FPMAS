#ifndef NONE_H
#define NONE_H

#include "../zoltan/zoltan_utils.h"
#include "../zoltan/zoltan_node_migrate.h"
#include "graph/parallel/proxy/proxy.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel::synchro {

	namespace modes {
		template<typename T, int N> class NoSyncMode;
	}

	namespace wrappers {

		/**
		 * Data wrapper for the NoSyncMode.
		 *
		 * @tparam T wrapped data type
		 * @tparam N layers count
		 */
		template<class T, int N> class NoSyncData : public SyncData<T,N,modes::NoSyncMode> {
			public:
				/**
				 * NoSyncData constructor.
				 */
				NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&) {};
				/**
				 * NoSyncData constructor.
				 */
				NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&, T&& data) :
					SyncData<T,N,modes::NoSyncMode>(std::forward<T>(data)) {};
				/**
				 * NoSyncData constructor.
				 */
				NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&, const T& data) :
					SyncData<T,N,modes::NoSyncMode>(data) {};
		};
	}

	namespace modes {

		/**
		 * Synchronisation mode that can be used as a template argument for
		 * a DistributedGraphBase, where no synchronization or overlapping zone
		 * is used.
		 *
		 * When a node is exported, its connections with nodes that are not
		 * exported on the same process are lost.
		 *
		 * In consequence, it is also implicitly impossible to create or
		 * destroy distant links.
		 */
		template<typename T, int N> class NoSyncMode : public SyncMode<NoSyncMode, wrappers::NoSyncData, T, N> {
			public:
				NoSyncMode(DistributedGraphBase<T, NoSyncMode, N>&);
		};

		/**
		 * NoSyncMode constructor.
		 *
		 * @param dg parent DistributedGraphBase
		 */
		template<typename T, int N> NoSyncMode<T,N>::NoSyncMode(DistributedGraphBase<T, NoSyncMode, N>& dg)
			: SyncMode<NoSyncMode, wrappers::NoSyncData, T, N>(zoltan_query_functions(
						&FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_no_sync<T, N>,
						&FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_no_sync<T, N>,
						NULL
						), dg) {};
	}
}
#endif
