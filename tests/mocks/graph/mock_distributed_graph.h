#ifndef MOCK_DISTRIBUTED_GRAPH_H
#define MOCK_DISTRIBUTED_GRAPH_H
#include "gmock/gmock.h"

#include "mock_graph_base.h"
#include "fpmas/api/graph/distributed_graph.h"

template<typename T, typename DistNode, typename DistEdge>
class MockDistributedGraph :
	public MockGraph<
		fpmas::api::graph::DistributedNode<T>,
		fpmas::api::graph::DistributedEdge<T>>,
	public fpmas::api::graph::DistributedGraph<T> {

		typedef fpmas::api::graph::DistributedGraph<T>
			GraphBase;
		public:
		typedef fpmas::api::graph::DistributedNode<T> NodeType;
		typedef fpmas::api::graph::DistributedEdge<T> EdgeType;
		using typename GraphBase::NodeMap;
		using typename GraphBase::EdgeMap;
		using typename GraphBase::NodeCallback;

		MOCK_METHOD(const fpmas::api::communication::MpiCommunicator&,
				getMpiCommunicator, (), (const, override));
		MOCK_METHOD(
				const fpmas::api::graph::LocationManager<T>&,
				getLocationManager, (), (const, override));

		NodeType* buildNode(T&& data) override {
			return buildNode_rv();
		}
		//MOCK_METHOD(NodeType*, buildNode, (const T&), (override));
		MOCK_METHOD(NodeType*, buildNode_rv, (), ());
		MOCK_METHOD(EdgeType*, link, (NodeType*, NodeType*, fpmas::api::graph::LayerId), (override));

		MOCK_METHOD(void, addCallOnSetLocal, (NodeCallback*), (override));
		MOCK_METHOD(void, addCallOnSetDistant, (NodeCallback*), (override));

		MOCK_METHOD(const DistributedId&, currentNodeId, (), (const, override));
		MOCK_METHOD(const DistributedId&, currentEdgeId, (), (const, override));

		MOCK_METHOD(void, removeNode, (NodeType*), (override));
		MOCK_METHOD(void, removeNode, (DistributedId), (override));
		MOCK_METHOD(void, unlink, (EdgeType*), (override));
		MOCK_METHOD(void, unlink, (DistributedId), (override));

		MOCK_METHOD(NodeType*, importNode, (NodeType*), (override));
		MOCK_METHOD(EdgeType*, importEdge, (EdgeType*), (override));

		MOCK_METHOD(void, balance, (fpmas::api::load_balancing::LoadBalancing<T>&), (override));
		MOCK_METHOD(void, balance, (
					fpmas::api::load_balancing::FixedVerticesLoadBalancing<T>&,
					fpmas::api::load_balancing::PartitionMap),
				(override));
		MOCK_METHOD(void, distribute, (fpmas::api::load_balancing::PartitionMap), (override));
		MOCK_METHOD(void, synchronize, (), (override));
	};

#endif
