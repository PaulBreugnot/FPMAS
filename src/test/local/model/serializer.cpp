#include "model/serializer.h"
#include "../mocks/model/mock_model.h"

using ::testing::AnyNumber;
using ::testing::WhenDynamicCastTo;
using ::testing::Not;
using ::testing::IsNull;

FPMAS_JSON_SERIALIZE_AGENT(MockAgent<4>, MockAgent<2>, MockAgent<12>)

//template<FPMAS::api::model::TypeId TYPE_ID>
//using MockAgent = FPMAS::serializer_test::MockAgent<TYPE_ID>;

TEST(AgentSerializer, to_json) {
	MockAgent<4>* agent_4 = new MockAgent<4>;
	MockAgent<12>* agent_12 = new MockAgent<12>;
	EXPECT_CALL(*agent_4, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_4, getField).WillRepeatedly(Return(7));
	EXPECT_CALL(*agent_4, groupId).WillRepeatedly(Return(2));
	EXPECT_CALL(*agent_12, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_12, getField).WillRepeatedly(Return(84));
	EXPECT_CALL(*agent_12, groupId).WillRepeatedly(Return(4));

	FPMAS::api::model::AgentPtr agent_ptr_4 {agent_4};
	FPMAS::api::model::AgentPtr agent_ptr_12 {agent_12};

	nlohmann::json j4 = agent_ptr_4;
	nlohmann::json j12 = agent_ptr_12;

	ASSERT_EQ(j4.at("type").get<FPMAS::api::model::TypeId>(), 4);
	ASSERT_EQ(j4.at("gid").get<FPMAS::api::model::GroupId>(), 2);
	ASSERT_EQ(j4.at("agent").at("mock").get<int>(), 7);

	ASSERT_EQ(j12.at("type").get<FPMAS::api::model::TypeId>(), 12);
	ASSERT_EQ(j12.at("gid").get<FPMAS::api::model::GroupId>(), 4);
	ASSERT_EQ(j12.at("agent").at("mock").get<int>(), 84);
}

TEST(AgentSerializer, from_json) {
	nlohmann::json j4;
	j4["type"] = 4;
	j4["gid"] = 2;
	j4["agent"]["field"] = 7;

	nlohmann::json j12;
	j12["type"] = 12;
	j12["gid"] = 4;
	j12["agent"]["field"] = 84;

	auto agent_4 = j4.get<FPMAS::api::model::AgentPtr>();
	auto agent_12 = j12.get<FPMAS::api::model::AgentPtr>();

	ASSERT_THAT(agent_4.get(), WhenDynamicCastTo<MockAgent<4>*>(Not(IsNull())));
	ASSERT_EQ(static_cast<MockAgent<4>*>(agent_4.get())->getField(), 7);
	ASSERT_EQ(agent_4->groupId(), 2);
	ASSERT_THAT(agent_12.get(), WhenDynamicCastTo<MockAgent<12>*>(Not(IsNull())));
	ASSERT_EQ(static_cast<MockAgent<12>*>(agent_12.get())->getField(), 84);
	ASSERT_EQ(agent_12->groupId(), 4);
}
