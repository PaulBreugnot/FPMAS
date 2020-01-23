#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::zoltan::ghost::obj_size_multi_fn;
using FPMAS::graph::parallel::zoltan::ghost::pack_obj_multi_fn;

using FPMAS::graph::parallel::DistributedGraph;

class Mpi_ZoltanGhostNodeMigrationFunctionsTest : public ::testing::Test {
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
			dg.buildNode(0ul, 1.5, 1);
			dg.buildNode(1ul, 2., -2);
		}

		void write_migration_sizes() {
			// Transfer nodes 0 and 85250
			FPMAS::graph::parallel::zoltan::utils::write_zoltan_id(0, &transfer_global_ids[0]);
			FPMAS::graph::parallel::zoltan::utils::write_zoltan_id(1, &transfer_global_ids[2]);

			obj_size_multi_fn<int>(
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

			pack_obj_multi_fn<int>(
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

TEST_F(Mpi_ZoltanGhostNodeMigrationFunctionsTest, obj_size_multi_test) {

	write_migration_sizes();

	json ghost0_str = *dg.getNodes().at(0);
	ASSERT_EQ(sizes[0], ghost0_str.dump().size() + 1);

	json ghost1_str = *dg.getNodes().at(1);
	ASSERT_EQ(sizes[1], ghost1_str.dump().size() + 1);
}

TEST_F(Mpi_ZoltanGhostNodeMigrationFunctionsTest, pack_obj_multi_test) {

	write_migration_sizes();
	write_communication_buffer();

	ASSERT_STREQ(
		&buf[0],
		R"({"data":1,"id":0,"weight":1.5})"
		);

	ASSERT_STREQ(
		&buf[idx[1]],
		R"({"data":-2,"id":1,"weight":2.0})"
		);

}

using FPMAS::graph::parallel::zoltan::ghost::unpack_obj_multi_fn;

TEST_F(Mpi_ZoltanGhostNodeMigrationFunctionsTest, unpack_obj_multi_test) {

	dg.getNode(0)->data()->get() = 8;
	dg.getNode(0)->setWeight(5.);

	dg.getNode(1)->data()->get() = 12;
	dg.getNode(1)->setWeight(4.);

	write_migration_sizes();
	write_communication_buffer();
	
	dg.getGhost().buildNode(*dg.getNode(0));
	dg.getGhost().buildNode(*dg.getNode(1));

	dg.removeNode(0);
	dg.removeNode(1);

	unpack_obj_multi_fn<int>(
		&dg,
		2,
		2,
		transfer_global_ids,
		sizes,
		idx,
		buf,
		&err);

	ASSERT_EQ(dg.getGhost().getNodes().at(0)->data()->get(), 8);
	ASSERT_EQ(dg.getGhost().getNodes().at(0)->getWeight(), 5.);

	ASSERT_EQ(dg.getGhost().getNodes().at(1)->data()->get(), 12);
	ASSERT_EQ(dg.getGhost().getNodes().at(1)->getWeight(), 4.);
}
