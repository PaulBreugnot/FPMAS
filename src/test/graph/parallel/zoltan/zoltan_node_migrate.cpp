#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/zoltan/zoltan_lb.h"
#include "graph/parallel/synchro/ghost_mode.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::graph::parallel::zoltan::utils::write_zoltan_id;

using FPMAS::graph::parallel::zoltan::node::obj_size_multi_fn;
using FPMAS::graph::parallel::zoltan::node::pack_obj_multi_fn;

using FPMAS::graph::parallel::synchro::SyncData;

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::graph::parallel::synchro::GhostMode;

class Mpi_ZoltanNodeMigrationFunctionsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

		// Migration
		unsigned int transfer_global_ids[4];
		int sizes[2];
		int idx[2];
		char buf[250];

		// Error code
		int err;

		void SetUp() override {
			dg.buildNode(0, 1., 0);
			dg.buildNode(2, 2., 1);
			dg.buildNode(85250, 3., 2);

			dg.link(0, 2, 0);
			dg.link(2, 0, 1);

			dg.link(0, 85250, 2);


		}

		void write_migration_sizes() {
			// Transfer nodes 0 and 85250
			write_zoltan_id(0, &transfer_global_ids[0]);
			write_zoltan_id(85250, &transfer_global_ids[2]);

			obj_size_multi_fn<int, 1, GhostMode>(
					&dg,
					2,
					0,
					2,
					transfer_global_ids,
					nullptr,
					sizes,
					&err
					);
		}

		void write_communication_buffer() {
			// Automatically generated by Zoltan in a real use case
			idx[0] = 0;
			idx[1] = sizes[0] + 1;

			// Unused
			int dest[2];

			pack_obj_multi_fn<int, 1, GhostMode>(
					&dg,
					2,
					0,
					2,
					transfer_global_ids,
					nullptr,
					dest,
					sizes,
					idx,
					buf,
					&err
					);
		}
};

TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, obj_size_multi_test) {

	write_migration_sizes();

	int origin = dg.getMpiCommunicator().getRank();
	json node1_str = *dg.getNodes().at(0);
	node1_str["origin"] = origin;
	node1_str["from"] = origin;
	ASSERT_EQ(sizes[0], node1_str.dump().size() + 1);

	json node2_str = *dg.getNodes().at(85250);
	node2_str["origin"] = origin;
	node2_str["from"] = origin;
	ASSERT_EQ(sizes[1], node2_str.dump().size() + 1);
}


TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, pack_obj_multi_test) {

	write_migration_sizes();
	write_communication_buffer();

	std::string origin = std::to_string(dg.getMpiCommunicator().getRank());
	// Decompose and check buffer data
	ASSERT_STREQ(
		&buf[0],
		std::string(R"({"data":0,"from":)" + origin + R"(,"id":0,"origin":)" + origin + R"(,"weight":1.0})").c_str()
		);

	ASSERT_STREQ(
		&buf[idx[1]],
		std::string(R"({"data":2,"from":)" + origin + R"(,"id":85250,"origin":)" + origin + R"(,"weight":3.0})").c_str()
		);
}

using FPMAS::graph::parallel::zoltan::node::unpack_obj_multi_fn;

TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, unpack_obj_multi_test) {

	write_migration_sizes();
	write_communication_buffer();

	DistributedGraph<int> g = DistributedGraph<int>();
	unpack_obj_multi_fn<int, 1, GhostMode>(
		&g,
		2,
		2,
		transfer_global_ids,
		sizes,
		idx,
		buf,
		&err);

	ASSERT_EQ(g.getNodes().size(), 2);

	ASSERT_EQ(g.getNodes().count(0), 1);
	auto node0 = g.getNodes().at(0);
	ASSERT_EQ(node0->getId(), 0);
	ASSERT_EQ(node0->data()->read(), 0);
	ASSERT_EQ(node0->getWeight(), 1.f);

	ASSERT_EQ(g.getNodes().count(85250ul), 1);
	//FPMAS::graph::base::Node<std::unique_ptr<SyncData<int>>, 1>* node1 = g.getNodes().at(85250);
	auto node1 = g.getNodes().at(85250);
	ASSERT_EQ(node1->getId(), 85250ul);
	ASSERT_EQ(node1->data()->read(), 2);
	ASSERT_EQ(node1->getWeight(), 3.f);

}

/*
 * post_migrate functions are not tested, because they only change the internal
 * structure of the DistributedGraph.
 * Their behavior are indirectly tested through the
 * DistributedGraph::distribute() functions in the different sync mode cases.
 */
