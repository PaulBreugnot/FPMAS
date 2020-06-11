#ifndef ZOLTAN_LOAD_BALANCING_H
#define ZOLTAN_LOAD_BALANCING_H

#include "api/load_balancing//load_balancing.h"
#include "utils/config.h"
#include "zoltan_cpp.h"

namespace FPMAS::load_balancing {

	/**
	 * The FPMAS::graph::zoltan namespace contains definitions of all the
	 * required Zoltan query functions to compute load balancing partitions
	 * based on the current graph nodes.
	 */
	namespace zoltan {
		template<typename T>
		using ConstNodeMap = typename api::load_balancing::FixedVerticesLoadBalancing<T>::ConstNodeMap;
		template<typename T>
		using PartitionMap = typename api::load_balancing::FixedVerticesLoadBalancing<T>::PartitionMap;

		void zoltan_config(Zoltan*);

		/**
		 * Convenient function to rebuild a regular node or arc id, as an
		 * unsigned long, from a ZOLTAN_ID_PTR global id array, that actually
		 * stores 2 unsigned int for each node unsigned long id.
		 * So, with our configuration, we use 2 unsigned int in Zoltan to
		 * represent each id. The `i` input parameter should correspond to the
		 * first part index of the node to query in the `global_ids` array. In
		 * consequence, `i` should actually always be even.
		 *
		 * @param global_ids an adress to a pair of Zoltan global ids
		 * @return rebuilt node id
		 */
		DistributedId read_zoltan_id(const ZOLTAN_ID_PTR global_ids);

		/**
		 * Writes zoltan global ids to the specified `global_ids` adress.
		 *
		 * The original `unsigned long` is decomposed into 2 `unsigned int` to
		 * fit the default Zoltan data structure. The written id can then be
		 * rebuilt using the node_id(const ZOLTAN_ID_PTR) function.
		 *
		 * @param id id to write
		 * @param global_ids adress to a Zoltan global_ids array
		 */
		void write_zoltan_id(DistributedId id, ZOLTAN_ID_PTR global_ids);

		/**
		 * Returns the number of nodes currently managed by the current processor
		 * (i.e. the number of nodes contained in the local DistributedGraphBase
		 * instance).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_OBJ_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param ierr Result : error code
		 * @return numbe of nodes managed by the current process
		 */
		template<typename T> int num_obj(void *data, int* ierr) {
			ConstNodeMap<T>*
				nodes = (ConstNodeMap<T>*) data;
			return nodes->size();
		}

		/**
		 * Lists node ids contained on the current process (i.e. in the
		 * DistributedGraphBase instance associated to the process).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_OBJ_LIST_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param global_ids Result : global ids assigned to processor
		 * @param local_ids Result : local ids assigned to processor (unused)
		 * @param wgt_dim Number of weights of each object, defined by OBJ_WEIGHT_DIM (should be 1)
		 * @param obj_wgts Result : weights list
		 * @param ierr Result : error code
		 */
		template<typename T> void obj_list(
				void *data,
				int num_gid_entries, 
				int num_lid_entries,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int wgt_dim,
				float *obj_wgts,
				int *ierr
				) {
			ConstNodeMap<T>* nodes = (ConstNodeMap<T>*) data;
			int i = 0;
			for(auto n : *nodes) {
				zoltan::write_zoltan_id(n.first, &global_ids[i * num_gid_entries]);
				obj_wgts[i++] = n.second->getWeight();
			}
		}

		/**
		 * Counts the number of outgoing arcs of each node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param num_obj number of objects IDs in global_ids
		 * @param global_ids Global IDs of object whose number of edges should be returned
		 * @param local_ids Same for local ids, unused
		 * @param num_edges Result : number of outgoing arc for each node
		 * @param ierr Result : error code
		 */
		template<typename T> void num_edges_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				int *ierr
				) {
			ConstNodeMap<T>* nodes = (ConstNodeMap<T>*) data;
			for(int i = 0; i < num_obj; i++) {
				auto node = nodes->at(
						zoltan::read_zoltan_id(&global_ids[i * num_gid_entries])
						);
				int count = 0;
				for(auto arc : node->getOutgoingArcs()) {
					if(nodes->count(arc->getTargetNode()->getId()) > 0) {
						count++;
					}
				}
				num_edges[i] = count;
			}
		}

		/**
		 * List node IDs connected to each node through outgoing arcs for each
		 * node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param num_obj number of objects IDs in global_ids
		 * @param global_ids Global IDs of object whose list of edges should be returned
		 * @param local_ids Same for local ids, unused
		 * @param num_edges number of edges of each corresponding node
		 * @param nbor_global_id Result : neighbor ids for each node
		 * @param nbor_procs Result : processor identifier of each neighbor in
		 * nbor_global_id
		 * @param wgt_dim number of edge weights
		 * @param ewgts Result : edge weight for each neighbor
		 * @param ierr Result : error code
		 */
		template<typename T> void edge_list_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				ZOLTAN_ID_PTR nbor_global_id,
				int *nbor_procs,
				int wgt_dim,
				float *ewgts,
				int *ierr) {

			ConstNodeMap<T>* nodes = (ConstNodeMap<T>*) data;

			int neighbor_index = 0;
			for (int i = 0; i < num_obj; ++i) {
				auto node = nodes->at(
						zoltan::read_zoltan_id(&global_ids[num_gid_entries * i])
						);
				for(auto arc : node->getOutgoingArcs()) {
					if(nodes->count(arc->getTargetNode()->getId()) > 0) {
						auto target = arc->getTargetNode();
						DistributedId targetId = target->getId(); 
						zoltan::write_zoltan_id(targetId, &nbor_global_id[neighbor_index * num_gid_entries]);

						nbor_procs[neighbor_index]
							= target->getLocation();

						ewgts[neighbor_index] = arc->getWeight();
						neighbor_index++;
					}
				}
			}
		}

		template<typename T> int num_fixed_obj_fn(
				void* data, int* ierr
				) {
			ConstNodeMap<T>* fixed_nodes = (ConstNodeMap<T>*) data;
			return fixed_nodes->size();
		}

		template<typename T> void fixed_obj_list_fn(
			void *data,
			int num_fixed_obj,
			int num_gid_entries,
			ZOLTAN_ID_PTR fixed_gids,
			int *fixed_parts,
			int *ierr
		) {
			PartitionMap<T>* fixed_nodes = (PartitionMap<T>*) data;
			int i = 0;
			for(auto fixed_node : *fixed_nodes) {
				zoltan::write_zoltan_id(fixed_node.first, &fixed_gids[i * num_gid_entries]);
				fixed_parts[i] = fixed_node.second;
				i++;
			}
		}
	}

	template<typename T>
		class ZoltanLoadBalancing : public FPMAS::api::load_balancing::FixedVerticesLoadBalancing<T> {
			public:
				typedef FPMAS::api::load_balancing::FixedVerticesLoadBalancing<T> base;
				using typename base::PartitionMap;
				using typename base::ConstNodeMap;

			private:
				//Zoltan instance
				Zoltan zoltan;

				// Number of arcs to export.
				int export_arcs_num;

				// Arc ids to export buffer.
				ZOLTAN_ID_PTR export_arcs_global_ids;
				// Arc export procs buffer.
				int* export_arcs_procs;

				void setUpZoltan();

				ConstNodeMap nodes;
				PartitionMap fixed_vertices;

			public:
				ZoltanLoadBalancing(MPI_Comm comm)
					: zoltan(comm) {
					setUpZoltan();
				}

				const std::unordered_map<DistributedId, T> getNodes() const {
					return nodes;
				}
				
				PartitionMap balance(
						ConstNodeMap nodes,
						PartitionMap fixed_vertices
						) override;

		};

	template<typename T> typename ZoltanLoadBalancing<T>::PartitionMap
		ZoltanLoadBalancing<T>::balance(
			ConstNodeMap nodes,
			PartitionMap fixed_vertices
			) {
			this->nodes = nodes;
			this->fixed_vertices = fixed_vertices;
			int changes;
			int num_lid_entries;
			int num_gid_entries;


			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int * import_procs;
			int * import_to_part;

			int export_node_num;
			ZOLTAN_ID_PTR export_node_global_ids;
			ZOLTAN_ID_PTR export_local_ids;
			int* export_node_procs;
			int* export_to_part;

			// Computes Zoltan partitioning
			this->zoltan.LB_Partition(
					changes,
					num_gid_entries,
					num_lid_entries,
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					export_node_num,
					export_node_global_ids,
					export_local_ids,
					export_node_procs,
					export_to_part
					);

			PartitionMap partition;
			for(int i = 0; i < export_node_num; i++) {
				partition[zoltan::read_zoltan_id(&export_node_global_ids[i * num_gid_entries])]
					= export_node_procs[i];
			}
			this->zoltan.LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan.LB_Free_Part(
					&export_node_global_ids,
					&export_local_ids,
					&export_node_procs,
					&export_to_part
					);

			this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
			return partition;
	}

	/*
	 * Initializes zoltan parameters and zoltan lb query functions.
	 */
	template<typename T> void ZoltanLoadBalancing<T>::setUpZoltan() {
		zoltan::zoltan_config(&this->zoltan);

		// Initializes Zoltan Node Load Balancing functions
		this->zoltan.Set_Num_Fixed_Obj_Fn(zoltan::num_fixed_obj_fn<T>, &this->fixed_vertices);
		this->zoltan.Set_Fixed_Obj_List_Fn(zoltan::fixed_obj_list_fn<T>, &this->fixed_vertices);
		this->zoltan.Set_Num_Obj_Fn(zoltan::num_obj<T>, &this->nodes);
		this->zoltan.Set_Obj_List_Fn(zoltan::obj_list<T>, &this->nodes);
		this->zoltan.Set_Num_Edges_Multi_Fn(zoltan::num_edges_multi_fn<T>, &this->nodes);
		this->zoltan.Set_Edge_List_Multi_Fn(zoltan::edge_list_multi_fn<T>, &this->nodes);
	}

}
#endif
