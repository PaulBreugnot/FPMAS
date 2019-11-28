#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_utils.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "utils/config.h"
#include "communication/communication.h"

#include "proxy.h"

using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::proxy::Proxy;

namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;
		template<class T> class GhostArc;
		template<class T> class GhostGraph;

		namespace zoltan {
			template<class T> int num_obj(void *data, int* ierr);
			template<class T> void obj_list(
					void *, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int, float *, int *
					);
			template<class T> void num_edges_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
					);
			template<class T> void edge_list_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, ZOLTAN_ID_PTR, int *, int, float *, int *
					);
			namespace node {
				template<class T> void obj_size_multi_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
						); 

				template<class T> void pack_obj_multi_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
						);

				template<class T> void unpack_obj_multi_fn(
						void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
						);
				template<class T> void post_migrate_pp_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *,
						int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *
						);
			}
		}

		/**
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 */
		template<class T> class DistributedGraph : public Graph<T> {
			friend void zoltan::node::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::arc::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::node::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
			friend void zoltan::arc::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
			friend void zoltan::node::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			friend void zoltan::arc::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			friend void zoltan::node::post_migrate_pp_fn<T>(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *,
      				int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *
					);


			private:
				MpiCommunicator mpiCommunicator;
				Zoltan* zoltan;
				Proxy proxy;
				GhostGraph<T> ghost;

				void setZoltanNodeMigration();
				void setZoltanArcMigration();

				// A set of node ids that are currently local and have been
				// imported from an other proc. Data memory of such a node
				// (ghost or local) have been allocated dynamically at
				// deserialization, and this set allows us to delete it when
				// those nodes are exported / removed.
				// std::set<unsigned long> importedNodeIds;

				// Serialization caches used to pack objects
				std::unordered_map<unsigned long, std::string> node_serialization_cache;
				std::unordered_map<unsigned long, std::string> arc_serialization_cache;

				/*
				 * Zoltan structures used to manage nodes and arcs migration
				 */
				// Node export buffer
				int* export_node_procs;

				// Arc migration buffers
				int export_arcs_num;
				ZOLTAN_ID_PTR export_arcs_global_ids;
				int* export_arcs_procs;

			

			public:
				DistributedGraph<T>();
				MpiCommunicator getMpiCommunicator() const;
				Proxy* getProxy();
				GhostGraph<T>* getGhost();

				void distribute();
				~DistributedGraph<T>();

		};

		/**
		 * DistributedGraph constructor.
		 *
		 * Instanciates and configure a new Zoltan instance accross all the
		 * available cores.
		 */
		template<class T> DistributedGraph<T>::DistributedGraph() : ghost(this) {
			proxy.setLocalProc(mpiCommunicator.getRank());

			this->zoltan = new Zoltan(mpiCommunicator.getMpiComm());
			FPMAS::config::zoltan_config(this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan::num_obj<T>, this);
			this->zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan::obj_list<T>, this);
			this->zoltan->Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan::num_edges_multi_fn<T>, this);
			this->zoltan->Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan::edge_list_multi_fn<T>, this);

			// Initializes Zoltan Arc migration buffers
			this->export_arcs_global_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			this->export_arcs_procs = (int*) std::malloc(0);
		}

		/**
		 * Returns the MpiCommunicator used by the Zoltan instance of this
		 * DistrDistributedGraph.
		 *
		 * @return mpiCommunicator associated to this graph
		 */
		template<class T> MpiCommunicator DistributedGraph<T>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}

		template<class T> Proxy* DistributedGraph<T>::getProxy() {
			return &this->proxy;
		}

		template<class T> GhostGraph<T>* DistributedGraph<T>::getGhost() {
			return &this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T> void DistributedGraph<T>::setZoltanNodeMigration() {
			zoltan->Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T>, this);
			zoltan->Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T>, this);
			zoltan->Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T>, this);
			zoltan->Set_Post_Migrate_PP_Fn(zoltan::node::post_migrate_pp_fn<T>, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T> void DistributedGraph<T>::setZoltanArcMigration() {
			zoltan->Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T>, this);
			zoltan->Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T>, this);
			zoltan->Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T>, this);
		}

		/**
		 * Distributes the graph accross the available cores using Zoltan.
		 *
		 * The graph is distributed using
		 * [Zoltan::LB_Partition](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_lb.html#Zoltan_LB_Partition)
		 * and
		 * [Zoltan::Migrate](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_mig.html#Zoltan_Migrate).
		 * Query functions used are defined in the FPMAS::graph::zoltan namespace.
		 *
		 * Because of the Zoltan behavior, the current process will block until
		 * all other involved process also call the distribute() function.
		 *
		 */
		// TODO: describe the full migration process
		template<class T> void DistributedGraph<T>::distribute() {
			int changes;
      		int num_gid_entries;
			int num_lid_entries;
			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int* export_to_part;

			int * import_procs;
			int * import_to_part;
			int num_export;
			ZOLTAN_ID_PTR export_global_ids;
			ZOLTAN_ID_PTR export_local_ids;
			
			// Computes Zoltan partitioning
			this->zoltan->LB_Partition(
				changes,
				num_gid_entries,
				num_lid_entries,
				num_import,
				import_global_ids,
				import_local_ids,
				import_procs,
				import_to_part,
				num_export,
				export_global_ids,
				export_local_ids,
				this->export_node_procs,
				export_to_part
				);

			if(changes > 0) {
				// Prepares Zoltan to migrate nodes
				this->setZoltanNodeMigration();

				// Migrate nodes from the load balancing
				// Arcs to export are computed in the post_migrate_pp_fn step.
				this->zoltan->Migrate(
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					num_export,
					export_global_ids,
					export_local_ids,
					this->export_node_procs,
					export_to_part
					);

				// Prepares Zoltan to migrate arcs associated to the exported
				// nodes.
				this->setZoltanArcMigration();

				// Unused buffer
				unsigned int export_arcs_local_ids[0];

				// Arcs to import
				int import_arcs_num;
				ZOLTAN_ID_PTR import_arcs_global_ids;
				ZOLTAN_ID_PTR import_arcs_local_ids;
				int* import_arcs_procs;
				int* import_arcs_parts;

				// Computes import/export arcs list from each local arc exports
				// lists, computing at node export.
				this->zoltan->Invert_Lists(
					this->export_arcs_num,
					this->export_arcs_global_ids,
					export_arcs_local_ids,
					this->export_arcs_procs,
					this->export_arcs_procs, // parts = procs
					import_arcs_num,
					import_arcs_global_ids,
					import_arcs_local_ids,
					import_arcs_procs,
					import_arcs_parts
					);

				// Performs arcs migration
				this->zoltan->Migrate(
					import_arcs_num,
					import_arcs_global_ids,
					import_arcs_local_ids,
					import_arcs_procs,
					import_arcs_parts,
					this->export_arcs_num,
					this->export_arcs_global_ids,
					export_arcs_local_ids,
					this->export_arcs_procs,
					this->export_arcs_procs // parts = procs
					);

				// The next steps will remove exported nodes from the local
				// graph, creating corresponding ghost nodes when necessary

				// Computes the set of ids of exported nodes
				std::set<unsigned long> exportedNodeIds;
				for(int i = 0; i < num_export; i++) {
					exportedNodeIds.insert(zoltan::utils::read_zoltan_id(&export_global_ids[i * num_gid_entries]));
				}

				// Computes which ghost nodes must be created.
				// For each exported node, a ghost node is created if and only
				// if at least one local node is still connected to the
				// exported node.
				std::vector<Node<T>*> ghostNodesToBuild;
				for(auto id : exportedNodeIds) {
					Node<T>* node = this->getNode(id);
					bool buildGhost = false;
					for(auto arc : node->getOutgoingArcs()) {
						if(exportedNodeIds.count(arc->getTargetNode()->getId()) == 0) {
							buildGhost = true;
							break;
						}
					}
					if(!buildGhost) {
						for(auto arc : node->getIncomingArcs()) {
							if(exportedNodeIds.count(arc->getSourceNode()->getId()) == 0) {
								buildGhost = true;
								break;
							}
						}
					}
					if(buildGhost) {
						ghostNodesToBuild.push_back(this->getNode(id));
					}
				}
				// Builds the ghost nodes
				for(auto node : ghostNodesToBuild) {
					this->getGhost()->buildNode(*node);
				}

				// Remove nodes and collect fossils
				Fossil<T> ghostFossils;
				for(auto id : exportedNodeIds) {
					ghostFossils.merge(this->removeNode(id));
				}

				// TODO : schema with fossils example
				// For info, see the Fossil and removeNode docs
				for(auto arc : ghostFossils.arcs) {
					this->ghost.clearArc(arc);
				}
			}

			// Frees the zoltan buffers
			this->zoltan->LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan->LB_Free_Part(
					&export_global_ids,
					&export_local_ids,
					&this->export_node_procs,
					&export_to_part
					);
		}

		/**
		 * DistributedGraph destructor.
		 *
		 * `deletes` ghost nodes and arcs, and data associated to Zoltan.
		 */
		template<class T> DistributedGraph<T>::~DistributedGraph() {
			std::free(this->export_arcs_global_ids);
			std::free(this->export_arcs_procs);
			delete this->zoltan;
		}


	}
}
#endif