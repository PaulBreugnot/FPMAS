#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"

#include "fpmas/load_balancing/zoltan_load_balancing.h"

using ::testing::AtLeast;
using ::testing::AnyNumber;

using fpmas::load_balancing::zoltan::read_zoltan_id;

using fpmas::load_balancing::zoltan::num_obj;
using fpmas::load_balancing::zoltan::obj_list;
using fpmas::load_balancing::zoltan::num_edges_multi_fn;
using fpmas::load_balancing::zoltan::edge_list_multi_fn;
using fpmas::load_balancing::zoltan::num_fixed_obj_fn;
using fpmas::load_balancing::zoltan::fixed_obj_list_fn;


class ZoltanFunctionsTest : public ::testing::Test {
	protected:
		DistributedId id1 {0, 1};
		std::vector<fpmas::api::graph::parallel::DistributedArc<int>*> outArcs1;
		MockDistributedArc<int>* mockArc1;
		MockDistributedArc<int>* mockArc2;

		DistributedId id2 {0, 2};
		std::vector<fpmas::api::graph::parallel::DistributedArc<int>*> outArcs2;

		DistributedId id3 {2, 3};
		std::vector<fpmas::api::graph::parallel::DistributedArc<int>*> outArcs3;
		MockDistributedArc<int>* mockArc3;

		std::unordered_map<
			DistributedId,
			MockDistributedNode<int>*,
			fpmas::api::graph::base::IdHash<DistributedId>
			> nodes;

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[6];
		unsigned int* local_ids;
		float weights[3];

		std::unordered_map<
			DistributedId, int, fpmas::api::graph::base::IdHash<DistributedId>
			> nodeIndex;

		// Edge lists
		int num_edges[3];

		// Error code
		int err;

		void SetUp() override {
			nodes[id1] = new MockDistributedNode(id1, 0, 1.f);
			nodes[id2] = new MockDistributedNode(id2, 1, 2.f);
			nodes[id3] = new MockDistributedNode(id3, 2, 3.f);

			// Mock arc id1 -> id2
			mockArc1 = new MockDistributedArc<int>(
					DistributedId(0, 0), 0, 1.5f
					);
			EXPECT_CALL(*mockArc1, getSourceNode).WillRepeatedly(Return(nodes[id1]));
			EXPECT_CALL(*mockArc1, getTargetNode).WillRepeatedly(Return(nodes[id2]));
			outArcs1.push_back(mockArc1);

			// Mock arc id1 -> id3
			mockArc2 = new MockDistributedArc<int>(
					DistributedId(0, 1), 1, 3.f
					);
			EXPECT_CALL(*mockArc2, getSourceNode).WillRepeatedly(Return(nodes[id1]));
			EXPECT_CALL(*mockArc2, getTargetNode).WillRepeatedly(Return(nodes[id3]));
			outArcs1.push_back(mockArc2);

			mockArc3 = new MockDistributedArc<int>(
					DistributedId(0, 2),
					0,
					2.f
					);
			EXPECT_CALL(*mockArc3, getSourceNode).WillRepeatedly(Return(nodes[id2]));
			EXPECT_CALL(*mockArc3, getTargetNode).WillRepeatedly(Return(nodes[id1]));
			outArcs2.push_back(mockArc3);

			// Assumes that nodes are iterated in the same order within
			// obj_list implementation
			int index = 0;
			for(auto node : nodes) {
				nodeIndex[node.first] = index++;
			}
		}

		void TearDown() override {
			for(auto node : nodes)
				delete node.second;
			for(auto arc : outArcs1)
				delete arc;
			for(auto arc : outArcs2)
				delete arc;
		}

		void write_zoltan_global_ids() {
			EXPECT_CALL(*nodes[id1], getWeight).Times(1);
			EXPECT_CALL(*nodes[id2], getWeight).Times(1);
			EXPECT_CALL(*nodes[id3], getWeight).Times(1);
			obj_list<int>(
					&nodes,
					2,
					0,
					global_ids,
					local_ids,
					1,
					weights,
					&err
					);
		}

		void write_zoltan_num_edges() {
			EXPECT_CALL(*nodes[id1], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs1));
			EXPECT_CALL(*nodes[id2], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs2));
			EXPECT_CALL(*nodes[id3], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs3));

			for(auto node : nodes) {
				EXPECT_CALL(*node.second, getId).Times(AnyNumber());
			}

			num_edges_multi_fn<int>(
					&nodes,
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

TEST_F(ZoltanFunctionsTest, num_obj) {
	int num = num_obj<int>(&nodes, &err);
	ASSERT_EQ(num, 3);
}

TEST_F(ZoltanFunctionsTest, obj_list_fn_test) {
	write_zoltan_global_ids();

	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id3]]), id3);
}


TEST_F(ZoltanFunctionsTest, obj_num_egdes_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	// Node 0 has 2 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id1]], 2);
	// Node 1 has 1 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id2]], 1);
	// Node 2 has 0 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id3]], 0);
}



TEST_F(ZoltanFunctionsTest, edge_list_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	EXPECT_CALL(*nodes[id1], getLocation())
		.WillRepeatedly(Return(0));
	EXPECT_CALL(*nodes[id2], getLocation())
		.WillRepeatedly(Return(3));
	EXPECT_CALL(*nodes[id3], getLocation())
		.WillRepeatedly(Return(5));

	EXPECT_CALL(*mockArc1, getWeight).Times(AtLeast(1));
	EXPECT_CALL(*mockArc2, getWeight).Times(AtLeast(1));
	EXPECT_CALL(*mockArc3, getWeight).Times(AtLeast(1));

	unsigned int nbor_global_id[6];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<int>(
			&nodes,
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

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset) * 2]), id2);
	ASSERT_EQ(ewgts[node1_offset], 1.5f);
	ASSERT_EQ(nbor_procs[node1_offset], 3);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset + 1) * 2]), id3);
	ASSERT_EQ(ewgts[node1_offset+1], 3.f);
	ASSERT_EQ(nbor_procs[node1_offset+1], 5);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * 2]), id1);
	ASSERT_EQ(ewgts[node2_offset], 2.f);
	ASSERT_EQ(nbor_procs[node2_offset], 0);
}

TEST_F(ZoltanFunctionsTest, num_fixed_obj) {
	int err;
	int num = num_fixed_obj_fn<int>(&nodes, &err);

	ASSERT_EQ(num, 3);
}

TEST_F(ZoltanFunctionsTest, fixed_obj_list) {
	fpmas::load_balancing::zoltan::PartitionMap<int> map;
	map[id1] = 7;
	map[id2] = 2;
	map[id3] = 3;

	std::unordered_map<DistributedId, int> node_index;
	// Assumes that nodes are iterated in the same order within
	// fixed_obj_list implementation
	int index = 0;
	for(auto node : map) {
		node_index[node.first] = index++;
	}

	unsigned int fixed_ids[6];
	int fixed_parts[3];
	fixed_obj_list_fn<int>(
			&map,
			3,
			2,
			fixed_ids,
			fixed_parts,
			&err
			);

	ASSERT_EQ(read_zoltan_id(&fixed_ids[2 * node_index[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&fixed_ids[2 * node_index[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&fixed_ids[2 * node_index[id3]]), id3);

	ASSERT_EQ(fixed_parts[node_index[id1]], 7);
	ASSERT_EQ(fixed_parts[node_index[id2]], 2);
	ASSERT_EQ(fixed_parts[node_index[id3]], 3);
}