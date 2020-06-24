#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "mock_mutex.h"

using ::testing::ReturnNew;

class MockDataSync : public fpmas::api::synchro::DataSync {
	public:
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncLinker : public fpmas::api::synchro::SyncLinker<T> {
	public:
		MOCK_METHOD(void, link, (const fpmas::api::graph::parallel::DistributedArc<T>*), (override));
		MOCK_METHOD(void, unlink, (const fpmas::api::graph::parallel::DistributedArc<T>*), (override));
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncModeRuntime : public fpmas::api::synchro::SyncModeRuntime<T> {
	public:
		MockSyncModeRuntime(
			fpmas::api::graph::parallel::DistributedGraph<T>&,
			fpmas::api::communication::MpiCommunicator&) {
			ON_CALL(*this, buildMutex).WillByDefault(ReturnNew<MockMutex<int>>());
		}
		typedef fpmas::api::graph::parallel::DistributedNode<T> NodeType;

		MOCK_METHOD(MockMutex<T>*, buildMutex, (NodeType*), (override));
		MOCK_METHOD(fpmas::api::synchro::SyncLinker<T>&, getSyncLinker, (), (override));
		MOCK_METHOD(fpmas::api::synchro::DataSync&, getDataSync, (), (override));

};

template<
	template<typename> class Mutex = MockMutex,
	template<typename> class Runtime = MockSyncModeRuntime>
using MockSyncMode = fpmas::api::synchro::SyncMode<Mutex, Runtime>;
#endif