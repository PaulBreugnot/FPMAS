#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "fpmas/api/graph/distributed_graph.h"
#include "mock_mutex.h"

using ::testing::ReturnNew;

class MockDataSync : public fpmas::api::synchro::DataSync {
	public:
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncLinker : public virtual fpmas::api::synchro::SyncLinker<T> {
	public:
		MOCK_METHOD(void, link, (fpmas::api::graph::DistributedEdge<T>*), (override));
		MOCK_METHOD(void, unlink, (fpmas::api::graph::DistributedEdge<T>*), (override));
		MOCK_METHOD(void, removeNode, (fpmas::api::graph::DistributedNode<T>*), (override));
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncMode : public fpmas::api::synchro::SyncMode<T> {
	public:
		MockSyncMode() {
			ON_CALL(*this, buildMutex).WillByDefault(ReturnNew<MockMutex<T>>());
		}
		MockSyncMode(
			fpmas::api::graph::DistributedGraph<T>&,
			fpmas::api::communication::MpiCommunicator&) {
			ON_CALL(*this, buildMutex).WillByDefault(ReturnNew<MockMutex<T>>());
		}

		MockSyncMode(MockSyncMode<T>&&) {};
		MockSyncMode& operator=(MockSyncMode<T>&&) {return *this;};

		typedef fpmas::api::graph::DistributedNode<T> NodeType;

		MOCK_METHOD(MockMutex<T>*, buildMutex, (NodeType*), (override));
		MOCK_METHOD(fpmas::api::synchro::SyncLinker<T>&, getSyncLinker, (), (override));
		MOCK_METHOD(fpmas::api::synchro::DataSync&, getDataSync, (), (override));

};
#endif
