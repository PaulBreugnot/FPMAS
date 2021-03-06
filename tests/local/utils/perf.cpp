#include "fpmas/utils/perf.h"

#include "gmock/gmock.h"

#include <thread>

using namespace testing;
using namespace fpmas::utils::perf;

struct MockProbe : public fpmas::api::utils::perf::Probe {
	MOCK_METHOD(std::string, label, (), (const, override));
	MOCK_METHOD(std::vector<Duration>&, durations, (), (override));
	MOCK_METHOD(const std::vector<Duration>&, durations, (), (const, override));
	MOCK_METHOD(void, start, (), (override));
	MOCK_METHOD(void, stop, (), (override));
};

TEST(Duration, json) {
	std::chrono::milliseconds duration(12);

	nlohmann::json j = duration;
	std::chrono::milliseconds unserial = j.get<std::chrono::milliseconds>();

	ASSERT_EQ(duration, unserial);
}

TEST(Probe, constructor) {
	Probe probe("hello");
	ASSERT_EQ(probe.label(), "hello");
	ASSERT_THAT(probe.durations(), IsEmpty());
}

TEST(Probe, measures) {
	Probe probe("");

	probe.start();
	probe.stop();
	ASSERT_THAT(probe.durations(), SizeIs(1));

	auto sleep_duration = std::chrono::milliseconds(10);
	probe.start();
	std::this_thread::sleep_for(sleep_duration);
	probe.stop();

	ASSERT_THAT(probe.durations(), SizeIs(2));
	ASSERT_GE(probe.durations()[1], sleep_duration);
}

TEST(Probe, conditional) {
	bool enable = false;

	Probe probe("", [&enable] () {return enable;});
	probe.start();
	probe.stop();
	ASSERT_THAT(probe.durations(), SizeIs(0));

	enable = true;
	probe.start();
	probe.stop();
	ASSERT_THAT(probe.durations(), SizeIs(1));
}

TEST(Monitor, default_counts_and_durations) {
	Monitor monitor;

	ASSERT_EQ(monitor.callCount("foo"), 0);
	ASSERT_EQ(monitor.totalDuration("foo").count(), 0);
}

TEST(Monitor, monitor) {
	Monitor monitor;

	std::vector<Duration> probe_1_durations {Duration(10)};
	NiceMock<MockProbe> probe_1;
	ON_CALL(probe_1, label)
		.WillByDefault(Return(std::string("probe_1")));
	ON_CALL(probe_1, durations())
		.WillByDefault(ReturnRef(probe_1_durations));

	std::vector<Duration> probe_2_durations {Duration(12), Duration(32), Duration(24)};
	NiceMock<MockProbe> probe_2;
	ON_CALL(probe_2, label)
		.WillByDefault(Return(std::string("probe_2")));
	ON_CALL(probe_2, durations())
		.WillByDefault(ReturnRef(probe_2_durations));

	monitor.commit(probe_1);
	ASSERT_THAT(probe_1_durations, IsEmpty());

	monitor.commit(probe_2);
	ASSERT_THAT(probe_2_durations, IsEmpty());

	ASSERT_EQ(monitor.callCount("probe_1"), 1);
	ASSERT_EQ(monitor.totalDuration("probe_1"), Duration(10));
	ASSERT_EQ(monitor.callCount("probe_2"), 3);
	ASSERT_EQ(monitor.totalDuration("probe_2"), Duration(12+32+24));
}
