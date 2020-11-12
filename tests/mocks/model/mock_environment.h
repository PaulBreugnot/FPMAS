#ifndef MOCK_ENVIRONMENT_H
#define MOCK_ENVIRONMENT_H

#include "gmock/gmock.h"
#include "fpmas/model/environment.h"
#include "mock_model.h"

template<typename CellType>
class MockRange : public fpmas::api::model::Range<CellType> {
	public:
		MOCK_METHOD(unsigned int, size, (), (const, override));
		MOCK_METHOD(bool, contains, (CellType*, CellType*), (const, override));

		virtual ~MockRange() {}
};

template<typename CellType>
class MockSpatialAgent : public fpmas::api::model::SpatialAgent<CellType>, public testing::NiceMock<detail::MockAgentBase<MockSpatialAgent<CellType>>> {
	public:
		MOCK_METHOD(void, act, (), (override));
		MOCK_METHOD(void, moveToCell, (CellType*), (override));
		MOCK_METHOD(void, initLocation, (CellType*), (override));
		MOCK_METHOD(CellType*, locationCell, (), (override));
		MOCK_METHOD(const fpmas::api::model::Range<CellType>&, mobilityRange, (), (const, override));
		MOCK_METHOD(const fpmas::api::model::Range<CellType>&, perceptionRange, (), (const, override));
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

#endif
