#ifndef MOCK_NODE_H
#define MOCK_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/id.h"
#include "api/graph/base/node.h"
#include "mock_arc.h"

using ::testing::Const;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;

using FPMAS::api::graph::base::Id;
using FPMAS::api::graph::base::LayerId;

class MockData {};

template<typename IdType, typename _ArcType>
class AbstractMockNode : public virtual FPMAS::api::graph::base::Node<
				 IdType, _ArcType
							 > {
	protected:
		IdType id;
		float weight = 1.;

	public:
		using typename FPMAS::api::graph::base::Node<
			IdType, _ArcType>::ArcType;

		AbstractMockNode() {
			// Mock initialized without id
		}
		AbstractMockNode(const IdType& id) : id(id) {
			setUpGetters();
		}
		AbstractMockNode(const IdType& id, float weight) : id(id), weight(weight) {
			setUpGetters();
		}

		MOCK_METHOD(IdType, getId, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const, override));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(const std::vector<ArcType*>, getIncomingArcs, (), (override));
		MOCK_METHOD(const std::vector<ArcType*>, getIncomingArcs, (LayerId), (override));

		MOCK_METHOD(const std::vector<ArcType*>, getOutgoingArcs, (), (override));
		MOCK_METHOD(const std::vector<ArcType*>, getOutgoingArcs, (LayerId), (override));

		MOCK_METHOD(void, linkIn, (ArcType*), (override));
		MOCK_METHOD(void, linkOut, (ArcType*), (override));

		MOCK_METHOD(void, unlinkIn, (ArcType*), (override));
		MOCK_METHOD(void, unlinkOut, (ArcType*), (override));

	private:
		void setUpGetters() {
			ON_CALL(*this, getId)
				.WillByDefault(ReturnPointee(&id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, getWeight)
				.WillByDefault(ReturnPointee(&weight));

		}
};


template<typename IdType>
class MockNode : public AbstractMockNode<IdType, MockArc<IdType>> {
	public:
		typedef AbstractMockNode<IdType, MockArc<IdType>> MockNodeBase;
		MockNode() : MockNodeBase() {}
		MockNode(const IdType& id)
			: MockNodeBase(id) {
		}
		MockNode(const IdType& id, float weight)
			: MockNodeBase(id, weight) {
		}
};
#endif
