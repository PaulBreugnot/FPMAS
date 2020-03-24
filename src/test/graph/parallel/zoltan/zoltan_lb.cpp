#include "gtest/gtest.h"

#include "communication/communication.h"
#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"
#include "graph/parallel/synchro/ghost_mode.h"

#include "utils/test.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::test::ASSERT_CONTAINS;

using FPMAS::graph::parallel::zoltan::utils::read_zoltan_id;
using FPMAS::graph::parallel::zoltan::utils::write_zoltan_id;

using FPMAS::graph::parallel::zoltan::obj_list;
using FPMAS::graph::parallel::zoltan::num_edges_multi_fn;

using FPMAS::graph::parallel::synchro::modes::GhostMode;

class Mpi_ZoltanFunctionsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[6];
		unsigned int local_ids[0];
		float weights[3];

		std::unordered_map<DistributedId, int> nodeIndex;

		// Edge lists
		int num_edges[3];

		// Error code
		int err;

		DistributedId id1;
		DistributedId id2;
		DistributedId id3;

		void SetUp() override {
			id1 = dg.buildNode(1., 0)->getId();
			id2 = dg.buildNode(2., 1)->getId();
			id3 = dg.buildNode(3., 2)->getId();

			dg.link(id1, id2)->getId();
			dg.link(id2, id1)->getId();

			dg.link(id1, id3)->getId();
		}

		void write_zoltan_global_ids() {
			obj_list<int, 1, GhostMode>(
					&dg,
					2,
					0,
					global_ids,
					local_ids,
					1,
					weights,
					&err
					);

			// Assumes that nodes are iterated in the same order within
			// obj_list implementation
			int index = 0;
			for(auto node : dg.getNodes()) {
				nodeIndex[node.first] = index++;
			}
		}

		void write_zoltan_num_edges() {
			num_edges_multi_fn<int, 1, GhostMode>(
					&dg,
					2,
					0,
					3,
					global_ids,
					local_ids,
					num_edges,
					&err
					);
		}
};


TEST_F(Mpi_ZoltanFunctionsTest, obj_list_fn_test) {

	write_zoltan_global_ids();

	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id3]]), id3);
}


TEST_F(Mpi_ZoltanFunctionsTest, obj_num_egdes_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	// Node 0 has 2 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id1]], 2);
	// Node 1 has 1 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id2]], 1);
	// Node 2 has 0 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id3]], 0);
}

using FPMAS::graph::parallel::zoltan::edge_list_multi_fn;


TEST_F(Mpi_ZoltanFunctionsTest, edge_list_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	unsigned int nbor_global_id[6];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<int, 1, GhostMode>(
			&dg,
			2,
			0,
			3,
			global_ids,
			local_ids,
			num_edges,
			nbor_global_id,
			nbor_procs,
			1,
			ewgts,
			&err
			);

	int node1_offset = nodeIndex[id1] < nodeIndex[id2] ? 0 : 1;
	int node2_offset = nodeIndex[id1] < nodeIndex[id2] ? 2 : 0;

	std::array<DistributedId, 2> node1_edges = {
		read_zoltan_id(&nbor_global_id[(node1_offset) * 2]),
		read_zoltan_id(&nbor_global_id[(node1_offset + 1) * 2]),
	};

	ASSERT_CONTAINS(id2, node1_edges);
	ASSERT_CONTAINS(id3, node1_edges);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * 2]), id1);

	for(int i = 0; i < 3; i++) {
		ASSERT_EQ(ewgts[i], 1.f);
		ASSERT_EQ(nbor_procs[i], dg.getMpiCommunicator().getRank());
	}

}


