#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/communication/communication.h"
#include "distributed_edge.h"
#include "distributed_node.h"
#include "location_manager.h"

#include "fpmas/graph/graph.h"

#define DIST_GRAPH_PARAMS\
	typename T,\
	typename SyncMode,\
	template<typename> class DistNodeImpl,\
	template<typename> class DistEdgeImpl,\
	typename MpiSetUp,\
	template<typename> class LocationManagerImpl

#define DIST_GRAPH_PARAMS_SPEC\
	T,\
	SyncMode,\
	DistNodeImpl,\
	DistEdgeImpl,\
	MpiSetUp,\
	LocationManagerImpl

namespace fpmas { namespace graph {
	
	using api::graph::LocationState;
	
	typedef api::communication::MpiSetUp<communication::MpiCommunicator, communication::TypedMpi> DefaultMpiSetUp;

	template<DIST_GRAPH_PARAMS>
	class DistributedGraph : 
		public Graph<
			api::graph::DistributedNode<T>,
			api::graph::DistributedEdge<T>>,
		public api::graph::DistributedGraph<T>
		 {
			public:
			typedef DistNodeImpl<T> DistNodeType;
			typedef DistEdgeImpl<T> DistEdgeType;
			typedef typename SyncMode::template SyncModeRuntimeType<T> SyncModeRuntimeType;

			static_assert(
					std::is_base_of<api::graph::DistributedNode<T>, DistNodeType>::value,
					"DistNodeImpl must implement api::graph::DistributedNode"
					);
			static_assert(
					std::is_base_of<api::graph::DistributedEdge<T>, DistEdgeType>::value,
					"DistEdgeImpl must implement api::graph::DistributedEdge"
					);
			typedef api::graph::DistributedGraph<T> DistGraphBase;

			public:
			typedef api::graph::DistributedNode<T> NodeType;
			typedef api::graph::DistributedEdge<T> EdgeType;
			using typename DistGraphBase::NodeMap;
			using typename DistGraphBase::PartitionMap;
			typedef typename MpiSetUp::communicator communicator;
			template<typename Data>
			using mpi = typename MpiSetUp::template mpi<Data>;
			using NodeCallback = typename DistGraphBase::NodeCallback;

			private:
			communicator mpi_communicator;
			mpi<DistributedId> id_mpi {mpi_communicator};
			mpi<std::pair<DistributedId, int>> location_mpi {mpi_communicator};
			mpi<NodePtrWrapper<T>> node_mpi {mpi_communicator};
			mpi<EdgePtrWrapper<T>> edge_mpi {mpi_communicator};

			LocationManagerImpl<T> location_manager;
			SyncModeRuntimeType sync_mode_runtime;

			std::vector<NodeCallback*> set_local_callbacks;
			std::vector<NodeCallback*> set_distant_callbacks;

			NodeType* buildNode(NodeType*);

			void setLocal(api::graph::DistributedNode<T>* node);
			void setDistant(api::graph::DistributedNode<T>* node);

			void triggerSetLocalCallbacks(api::graph::DistributedNode<T>* node) {
				for(auto callback : set_local_callbacks)
					callback->call(node);
			}

			void triggerSetDistantCallbacks(api::graph::DistributedNode<T>* node) {
				for(auto callback : set_distant_callbacks)
					callback->call(node);
			}

			void clearDistantNodes();
			void clearNode(NodeType*);

			protected:
				DistributedId node_id;
				DistributedId edge_id;

			public:
			DistributedGraph() :
				location_manager(mpi_communicator, id_mpi, location_mpi),
				sync_mode_runtime(*this, mpi_communicator) {
				// Initialization in the body of this (derived) class of the
				// (base) fields nodeId and edgeId, to ensure that
				// mpi_communicator is initialized (as a field of this derived
				// class)
				this->node_id = DistributedId(mpi_communicator.getRank(), 0);
				this->edge_id = DistributedId(mpi_communicator.getRank(), 0);
			}

			const communicator& getMpiCommunicator() const override {
				return mpi_communicator;
			};
			communicator& getMpiCommunicator() {
				return mpi_communicator;
			};

			const mpi<NodePtrWrapper<T>>& getNodeMpi() const {return node_mpi;}
			const mpi<EdgePtrWrapper<T>>& getEdgeMpi() const {return edge_mpi;}

			const DistributedId& currentNodeId() const override {return node_id;}
			const DistributedId& currentEdgeId() const override {return edge_id;}

			NodeType* importNode(NodeType* node) override;
			EdgeType* importEdge(EdgeType* edge) override;

			const SyncModeRuntimeType& getSyncModeRuntime() const {return sync_mode_runtime;}

			const LocationManagerImpl<T>&
				getLocationManager() const override {return location_manager;}

			void removeNode(NodeType*) override {};
			void removeNode(DistributedId id) override {
				this->removeNode(this->getNode(id));
			}

			void unlink(EdgeType*) override;
			void unlink(DistributedId id) override {
				this->unlink(this->getEdge(id));
			}

			void balance(api::load_balancing::LoadBalancing<T>& load_balancing) override {
				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Balancing graph (%lu nodes, %lu edges)",
						this->getNodes().size(), this->getEdges().size());

				typename api::load_balancing::LoadBalancing<T>::ConstNodeMap node_map;
				for(auto node : this->location_manager.getLocalNodes()) {
					node_map.insert(node);
				}
				this->distribute(load_balancing.balance(node_map));

				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Graph balanced : %lu nodes, %lu edges",
						this->getNodes().size(), this->getEdges().size());
			};

			void balance(api::load_balancing::FixedVerticesLoadBalancing<T>& load_balancing, PartitionMap fixed_nodes) override {
				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Balancing graph (%lu nodes, %lu edges)",
						this->getNodes().size(), this->getEdges().size());

				typename api::load_balancing::LoadBalancing<T>::ConstNodeMap node_map;
				for(auto node : this->getNodes()) {
					node_map.insert(node);
				}
				this->distribute(load_balancing.balance(node_map, fixed_nodes));

				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Graph balanced : %lu nodes, %lu edges",
						this->getNodes().size(), this->getEdges().size());
			};

			void distribute(PartitionMap partition) override;

			void synchronize() override;


			//NodeType* buildNode(const T&) override;
			NodeType* buildNode(T&& = std::move(T())) override;

			EdgeType* link(NodeType* const src, NodeType* const tgt, api::graph::LayerId layer) override;

			void addCallOnSetLocal(NodeCallback* callback) override {
				set_local_callbacks.push_back(callback);
			};

			void addCallOnSetDistant(NodeCallback* callback) override {
				set_distant_callbacks.push_back(callback);
			};

			~DistributedGraph();
		};

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setLocal(api::graph::DistributedNode<T>* node) {
			location_manager.setLocal(node);
			triggerSetLocalCallbacks(node);
		}

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setDistant(api::graph::DistributedNode<T>* node) {
			location_manager.setDistant(node);
			triggerSetDistantCallbacks(node);
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(EdgeType* edge) {
		auto src = edge->getSourceNode();
		auto tgt = edge->getTargetNode();
		src->mutex().lockShared();
		tgt->mutex().lockShared();

		sync_mode_runtime.getSyncLinker().unlink(static_cast<DistEdgeType*>(edge));

		this->erase(edge);

		src->mutex().unlockShared();
		tgt->mutex().unlockShared();
	}

	template<DIST_GRAPH_PARAMS>
	typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
	DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(NodeType* node) {
		FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing node %s...", ID_C_STR(node->getId()));
		// The input node must be a temporary dynamically allocated object.
		// A representation of the node might already be contained in the
		// graph, if it were already built as a "distant" node.

		if(this->getNodes().count(node->getId())==0) {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Inserting new LOCAL node %s.", ID_C_STR(node->getId()));
			// The node is not contained in the graph, we need to build a new
			// one.
			// But instead of completely building a new node, we can re-use the
			// temporary input node.
			this->insert(node);
			setLocal(node);
			node->setMutex(sync_mode_runtime.buildMutex(node));
			return node;
		}
		FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Replacing existing DISTANT node %s.", ID_C_STR(node->getId()));
		// A representation of the node was already contained in the graph.
		// We just need to update its state.

		// Set local representation as local
		auto local_node = this->getNode(node->getId());
		local_node->data() = std::move(node->data());
		local_node->setWeight(node->getWeight());
		setLocal(local_node);

		// Deletes unused temporary input node
		delete node;

		return local_node;
	}

	template<DIST_GRAPH_PARAMS>
	typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
	DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importEdge(EdgeType* edge) {
		FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing edge %s (from %s to %s)...",
				ID_C_STR(edge->getId()),
				ID_C_STR(edge->getSourceNode()->getId()),
				ID_C_STR(edge->getTargetNode()->getId())
				);
		// The input edge must be a dynamically allocated object, with temporary
		// dynamically allocated nodes as source and target.
		// A representation of the imported edge might already be present in the
		// graph, for example if it has already been imported as a "distant"
		// edge with other nodes at other epochs.

		if(this->getEdges().count(edge->getId())==0) {
			// The edge does not belong to the graph : a new one must be built.

			DistributedId srcId = edge->getSourceNode()->getId();
			NodeType* src;
			DistributedId tgtId =  edge->getTargetNode()->getId();
			NodeType* tgt;

			LocationState edgeLocationState = LocationState::LOCAL;

			if(this->getNodes().count(srcId) > 0) {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing source %s", ID_C_STR(srcId));
				// The source node is already contained in the graph
				src = this->getNode(srcId);
				if(src->state() == LocationState::DISTANT) {
					// At least src is DISTANT, so the imported edge is
					// necessarily DISTANT.
					edgeLocationState = LocationState::DISTANT;
				}
				// Deletes the temporary source node
				delete edge->getSourceNode();

				// Links the temporary edge with the src contained in the graph
				edge->setSourceNode(src);
				src->linkOut(edge);
			} else {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT source %s", ID_C_STR(srcId));
				// The source node is not contained in the graph : it must be
				// built as a DISTANT node.
				edgeLocationState = LocationState::DISTANT;

				// Instead of building a new node, we re-use the temporary
				// source node.
				src = edge->getSourceNode();
				this->insert(src);
				setDistant(src);
				src->setMutex(sync_mode_runtime.buildMutex(src));
			}
			if(this->getNodes().count(tgtId) > 0) {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing target %s", ID_C_STR(tgtId));
				// The target node is already contained in the graph
				tgt = this->getNode(tgtId);
				if(tgt->state() == LocationState::DISTANT) {
					// At least src is DISTANT, so the imported edge is
					// necessarily DISTANT.
					edgeLocationState = LocationState::DISTANT;
				}
				// Deletes the temporary target node
				delete edge->getTargetNode();

				// Links the temporary edge with the tgt contained in the graph
				edge->setTargetNode(tgt);
				tgt->linkIn(edge);
			} else {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT target %s", ID_C_STR(tgtId));
				// The target node is not contained in the graph : it must be
				// built as a DISTANT node.
				edgeLocationState = LocationState::DISTANT;

				// Instead of building a new node, we re-use the temporary
				// target node.
				tgt = edge->getTargetNode();
				this->insert(tgt);
				setDistant(tgt);
				tgt->setMutex(sync_mode_runtime.buildMutex(tgt));
			}
			// Finally, insert the temporary edge into the graph.
			edge->setState(edgeLocationState);
			this->insert(edge);
			return edge;
		} // if (graph.count(edge_id) > 0)

		// A representation of the edge is already present in the graph : it is
		// useless to insert it again. We just need to update its state.

		auto local_edge = this->getEdge(edge->getId());
		if(local_edge->getSourceNode()->state() == LocationState::LOCAL
				&& local_edge->getTargetNode()->state() == LocationState::LOCAL) {
			local_edge->setState(LocationState::LOCAL);
		}

		// Completely deletes temporary items, nothing is re-used
		delete edge->getSourceNode();
		delete edge->getTargetNode();
		delete edge;

		return local_edge;
	}

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(NodeType* node) {
				this->insert(node);
				setLocal(node);
				location_manager.addManagedNode(node, mpi_communicator.getRank());
				node->setMutex(sync_mode_runtime.buildMutex(node));
				//sync_mode_runtime.setUp(node->getId(), dynamic_cast<typename SyncMode::template MutexType<T>&>(node->mutex()));
				return node;
			}

	/*
	 *template<DIST_GRAPH_PARAMS>
	 *    typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
	 *        DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(const T& data) {
	 *            return buildNode(new DistNodeType(
	 *                    this->node_id++, data
	 *                    ));
	 *        }
	 */

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(T&& data) {
				return buildNode(new DistNodeType(
						this->node_id++, std::move(data)
						));
			}

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::link(NodeType* const src, NodeType* const tgt, api::graph::LayerId layer) {
				// Locks source and target
				src->mutex().lockShared();
				tgt->mutex().lockShared();

				// Builds the new edge
				auto edge = new DistEdgeType(
						this->edge_id++, layer
						);
				edge->setSourceNode(src);
				src->linkOut(edge);
				edge->setTargetNode(tgt);
				tgt->linkIn(edge);

				edge->setState(
						src->state() == LocationState::LOCAL && tgt->state() == LocationState::LOCAL ?
						LocationState::LOCAL :
						LocationState::DISTANT
						);
				sync_mode_runtime.getSyncLinker().link(edge);

				// Inserts the edge in the Graph
				this->insert(edge);

				// Unlocks source and target
				src->mutex().unlockShared();
				tgt->mutex().unlockShared();

				return edge;
			}


	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::distribute(PartitionMap partition) {
			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"Distributing graph...");
			std::string partition_str = "\n";
			for(auto item : partition) {
				std::string str = ID_C_STR(item.first);
				str.append(" : " + std::to_string(item.second) + "\n");
				partition_str.append(str);
			}
			FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Partition : %s", partition_str.c_str());

			sync_mode_runtime.getSyncLinker().synchronize();

			// Builds node and edges export maps
			std::vector<NodeType*> exported_nodes;
			std::unordered_map<int, std::vector<NodePtrWrapper<T>>> node_export_map;
			std::unordered_map<int, std::set<DistributedId>> edge_ids_to_export;
			std::unordered_map<int, std::vector<EdgePtrWrapper<T>>> edge_export_map;
			for(auto item : partition) {
				if(this->getNodes().count(item.first) > 0) {
					if(item.second != mpi_communicator.getRank()) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH",
								"Exporting node %s to %i", ID_C_STR(item.first), item.second);
						auto node_to_export = this->getNode(item.first);
						exported_nodes.push_back(node_to_export);
						node_export_map[item.second].emplace_back(node_to_export);
						for(auto edge :  node_to_export->getIncomingEdges()) {
							// Insert or replace in the IDs set
							edge_ids_to_export[item.second].insert(edge->getId());
						}
						for(auto edge :  node_to_export->getOutgoingEdges()) {
							// Insert or replace in the IDs set
							edge_ids_to_export[item.second].insert(edge->getId());
						}
					}
				}
			}
			// Ensures that each edge is exported once to each process
			for(auto list : edge_ids_to_export) {
				for(auto id : list.second) {
					edge_export_map[list.first].emplace_back(this->getEdge(id));
				}
			}

			// Serialize and export / import nodes
			auto node_import = node_mpi.migrate(node_export_map);

			// Serialize and export / import edges
			auto edge_import = edge_mpi.migrate(edge_export_map);

			NodeMap imported_nodes;
			for(auto& import_node_list_from_proc : node_import) {
				for(auto& imported_node : import_node_list_from_proc.second) {
					auto node = this->importNode(imported_node);
					imported_nodes.insert({node->getId(), node});
				}
			}
			for(auto& import_edge_list_from_proc : edge_import) {
				for(auto& imported_edge : import_edge_list_from_proc.second) {
					this->importEdge(imported_edge);
				}
			}

			for(auto node : exported_nodes) {
				setDistant(node);
			}

			location_manager.updateLocations(imported_nodes);

			FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Clearing exported nodes...");
			for(auto node : exported_nodes) {
				clearNode(node);
			}
			FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Exported nodes cleared.");

			sync_mode_runtime.getDataSync().synchronize();

			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"End of distribution.");
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::synchronize() {
			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"Synchronizing graph...");

			sync_mode_runtime.getSyncLinker().synchronize();

			clearDistantNodes();

			sync_mode_runtime.getDataSync().synchronize();

			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"End of graph synchronization.");
		}

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clearDistantNodes() {
			NodeMap distant_nodes = location_manager.getDistantNodes();
			for(auto node : distant_nodes)
				clearNode(node.second);
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clearNode(NodeType* node) {
			DistributedId id = node->getId();
			FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Clearing node %s...",
					ID_C_STR(id)
					);

			bool eraseNode = true;
			std::set<EdgeType*> obsoleteEdges;
			for(auto edge : node->getIncomingEdges()) {
				if(edge->getSourceNode()->state()==LocationState::LOCAL) {
					eraseNode = false;
				} else {
					obsoleteEdges.insert(edge);
				}
			}
			for(auto edge : node->getOutgoingEdges()) {
				if(edge->getTargetNode()->state()==LocationState::LOCAL) {
					eraseNode = false;
				} else {
					obsoleteEdges.insert(edge);
				}
			}
			if(eraseNode) {
				FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Erasing obsolete node %s.",
					ID_C_STR(node->getId())
					);
				location_manager.remove(node);
				this->erase(node);
			} else {
				for(auto edge : obsoleteEdges) {
					FPMAS_LOGD(
						mpi_communicator.getRank(),
						"DIST_GRAPH", "Erasing obsolete edge %s (%p) (from %s to %s)",
						ID_C_STR(node->getId()), edge,
						ID_C_STR(edge->getSourceNode()->getId()),
						ID_C_STR(edge->getTargetNode()->getId())
						);
					this->erase(edge);
				}
			}
			FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Node %s cleared.",
					ID_C_STR(id)
					);
		}

	template<DIST_GRAPH_PARAMS>
		DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::~DistributedGraph() {
			for(auto callback : set_local_callbacks)
				delete callback;
			for(auto callback : set_distant_callbacks)
				delete callback;
		}
}}
#endif