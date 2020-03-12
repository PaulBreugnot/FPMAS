#include "gtest/gtest.h"
#include "agent/agent.h"

using FPMAS::agent::Agent;

class Sheep;
class Wolf;

typedef Agent<Wolf, Sheep> PreyPredAgent;

class Wolf : public PreyPredAgent {
	public:
		void act() {}
};
void to_json(json& j, const Wolf& w) {
	j["role"] = "WOLF";
}
void from_json(const json&, Wolf&) {}

class Sheep : public PreyPredAgent {
	public:
		void act() {}
};

void to_json(json& j, const Sheep& w) {
	j["role"] = "SHEEP";
}
void from_json(const json&, Sheep&) {}

typedef nlohmann::adl_serializer<std::unique_ptr<Agent<Wolf, Sheep>>> test_serializer;

TEST(AgentSerializerTest, test_type_maps) {
	ASSERT_EQ(test_serializer::type_id_map.size(), 2);
	ASSERT_EQ(test_serializer::id_type_map.size(), 2);

	unsigned long wolf_id = test_serializer::type_id_map.at(typeid(Wolf));
	ASSERT_EQ(
		test_serializer::id_type_map.at(wolf_id).get(), // Type associated to the generated id
		((nlohmann::TypeInfoRef) typeid(Wolf)).get() // Actual type
		);

	unsigned long sheep_id = test_serializer::type_id_map.at(typeid(Sheep));
	ASSERT_EQ(
		test_serializer::id_type_map.at(sheep_id).get(),
		((nlohmann::TypeInfoRef) typeid(Sheep)).get()
		);
}

TEST(AgentSerializerTest, serialize) {
	std::unique_ptr<PreyPredAgent> wolf_ptr(new Wolf());
	std::unique_ptr<PreyPredAgent> sheep_ptr(new Sheep());

	json j_wolf = wolf_ptr;
	json j_sheep = sheep_ptr;

	ASSERT_EQ(j_wolf.size(), 2);
	ASSERT_EQ(j_wolf.at("type").get<unsigned long>(), test_serializer::type_id_map.at(typeid(Wolf)));
	ASSERT_EQ(j_wolf.at("agent").at("role").get<std::string>(), "WOLF");
	
	ASSERT_EQ(j_sheep.size(), 2);
	ASSERT_EQ(j_sheep.at("type").get<unsigned long>(), test_serializer::type_id_map.at(typeid(Sheep)));
	ASSERT_EQ(j_sheep.at("agent").at("role").get<std::string>(), "SHEEP");
}

TEST(AgentSerializerTest, unserialize) {
	std::unique_ptr<PreyPredAgent> agent_ptr;
	json j_wolf = R"({"agent":{"role":"WOLF"},"type":0})"_json;

	agent_ptr = j_wolf.get<std::unique_ptr<PreyPredAgent>>();
	ASSERT_EQ(typeid(*agent_ptr), typeid(Wolf));

	json j_sheep = R"({"agent":{"role":"SHEEP"},"type":1})"_json;

	agent_ptr = j_sheep.get<std::unique_ptr<PreyPredAgent>>();
	ASSERT_EQ(typeid(*agent_ptr), typeid(Sheep));

}