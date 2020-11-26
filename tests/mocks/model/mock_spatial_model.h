#ifndef MOCK_ENVIRONMENT_H
#define MOCK_ENVIRONMENT_H

#include "gmock/gmock.h"
#include "fpmas/model/spatial_model.h"
#include "mock_model.h"

template<typename CellType>
class MockRange : public fpmas::api::model::Range<CellType> {
	public:
		MOCK_METHOD(std::size_t, size, (), (const, override));
		MOCK_METHOD(bool, contains, (CellType*, CellType*), (const, override));

		virtual ~MockRange() {}
};

class MockSpatialAgent : public fpmas::api::model::SpatialAgent, public testing::NiceMock<detail::MockAgentBase<MockSpatialAgent>> {
	public:
		MOCK_METHOD(void, act, (), (override));
		MOCK_METHOD(void, moveTo, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(void, moveTo, (fpmas::api::graph::DistributedId), (override));
		MOCK_METHOD(void, initLocation, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(fpmas::api::model::Cell*, locationCell, (), (const, override));
		MOCK_METHOD(fpmas::api::graph::DistributedId, locationId, (), (const, override));
		MOCK_METHOD(void, handleNewMove, (), (override));
		MOCK_METHOD(void, handleNewPerceive, (), (override));

		virtual ~MockSpatialAgent() {}
};

class MockCellBase : public fpmas::model::CellBase, public testing::NiceMock<detail::MockAgentBase<MockCellBase>> {
	public:
		MOCK_METHOD(void, act, (), (override));

		virtual ~MockCellBase() {}
};

class MockCell : public virtual fpmas::api::model::Cell, public testing::NiceMock<detail::MockAgentBase<MockCell>> {
	public:
		MOCK_METHOD(void, act, (), (override));

		MOCK_METHOD(std::vector<fpmas::api::model::Cell*>, neighborhood,
				(), (override));
		MOCK_METHOD(void, handleNewLocation, (), (override));
		MOCK_METHOD(void, handleMove, (), (override));
		MOCK_METHOD(void, handlePerceive, (), (override));
		MOCK_METHOD(void, updatePerceptions, (), (override));

		virtual ~MockCell() {}
};

class MockSpatialModel : public fpmas::api::model::SpatialModel, public testing::NiceMock<MockModel> {
	public:
		MOCK_METHOD(void, add, (fpmas::api::model::SpatialAgentBase*), (override));
		MOCK_METHOD(void, add, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(std::vector<fpmas::api::model::Cell*>, cells, (), (override));
		MOCK_METHOD(fpmas::api::model::AgentGroup&, buildMoveGroup,
				(fpmas::model::GroupId, const fpmas::api::model::Behavior&), (override));

		MOCK_METHOD(fpmas::api::scheduler::JobList, distributedMoveAlgorithm, (), (override));

};

class MockSpatialAgentFactory : public fpmas::api::model::SpatialAgentFactory {
	public:
		MOCK_METHOD(fpmas::api::model::SpatialAgent*, build, (), (override));
};

template<typename CellType>
class MockSpatialAgentMapping : public fpmas::api::model::SpatialAgentMapping<CellType> {
	public:
		MOCK_METHOD(std::size_t, countAt, (CellType*), (override));
};

#endif