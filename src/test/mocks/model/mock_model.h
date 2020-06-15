#ifndef MOCK_MODEL_H
#define MOCK_MODEL_H
#include "gmock/gmock.h"
#include "api/model/model.h"

using ::testing::Return;
using ::testing::AnyNumber;

template<FPMAS::api::model::TypeId _TYPE_ID = 0>
class MockAgent : public FPMAS::api::model::Agent {
	public:
		inline static const FPMAS::api::model::TypeId TYPE_ID = _TYPE_ID;

		MOCK_METHOD(FPMAS::api::model::GroupId, groupId, (), (const, override));
		MOCK_METHOD(FPMAS::api::model::TypeId, typeId, (), (const, override));
		MOCK_METHOD(FPMAS::api::model::AgentNode*, node, (), (override));
		MOCK_METHOD(const FPMAS::api::model::AgentNode*, node, (), (const, override));
		MOCK_METHOD(void, setNode, (FPMAS::api::model::AgentNode*), (override));

		MOCK_METHOD(FPMAS::api::model::AgentTask*, task, (), (override));
		MOCK_METHOD(const FPMAS::api::model::AgentTask*, task, (), (const, override));
		MOCK_METHOD(void, setTask, (FPMAS::api::model::AgentTask*), (override));

		MOCK_METHOD(void, act, (), (override));

		// A fake custom agent field
		MOCK_METHOD(void, setField, (int), ());
		MOCK_METHOD(int, getField, (), (const));

		MockAgent() {
			ON_CALL(*this, typeId).WillByDefault(Return(_TYPE_ID));
		}

		MockAgent(int field) : MockAgent() {
			EXPECT_CALL(*this, getField).Times(AnyNumber())
				.WillRepeatedly(Return(field));
		}
};

class MockModel : public FPMAS::api::model::Model {
	public:
		MOCK_METHOD(AgentGraph&, graph, (), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Scheduler&, scheduler, (), (override));
		MOCK_METHOD(FPMAS::api::runtime::Runtime&, runtime, (), (override));
		MOCK_METHOD(const FPMAS::api::scheduler::Job&, loadBalancingJob, (), (const, override));

		MOCK_METHOD(FPMAS::api::model::AgentGroup&, buildGroup, (), (override));
		MOCK_METHOD(FPMAS::api::model::AgentGroup&, getGroup, (FPMAS::api::model::GroupId), (override));

		MOCK_METHOD((const std::unordered_map<FPMAS::api::model::GroupId, FPMAS::api::model::AgentGroup*>&),
				groups, (), (const, override));
};

namespace nlohmann {
	template<FPMAS::api::model::TypeId TYPE_ID>
	using MockAgentPtr = FPMAS::api::utils::VirtualPtrWrapper<MockAgent<TYPE_ID>>;

	template<FPMAS::api::model::TypeId TYPE_ID>
		struct adl_serializer<MockAgentPtr<TYPE_ID>> {
			static void to_json(json& j, const MockAgentPtr<TYPE_ID>& data) {
				j["mock"] = data->getField();
			}

			static void from_json(const json& j, MockAgentPtr<TYPE_ID>& ptr) {
				ptr = MockAgentPtr<TYPE_ID>(new MockAgent<TYPE_ID>(j.at("field").get<int>()));
			}
		};
}

#endif