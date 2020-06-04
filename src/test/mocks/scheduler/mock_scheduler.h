#ifndef MOCK_SCHEDULER_H
#define MOCK_SCHEDULER_H

#include "gmock/gmock.h"
#include "api/scheduler/scheduler.h"

class MockTask : public FPMAS::api::scheduler::Task {
	public:
		MOCK_METHOD(void, execute, (), (override));
};

class MockJob : public FPMAS::api::scheduler::Job {
	public:
		MOCK_METHOD(FPMAS::JID, id, (), (const, override));
		MOCK_METHOD(void, add, (FPMAS::api::scheduler::Task*), (override));
		MOCK_METHOD(const std::vector<FPMAS::api::scheduler::Task*>&, tasks, (), (const, override));

		MOCK_METHOD(void, setBegin, (FPMAS::api::scheduler::Task*), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Task*, getBegin, (), (const, override));

		MOCK_METHOD(void, setEnd, (FPMAS::api::scheduler::Task*), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Task*, getEnd, (), (const, override));

};

class MockEpoch : public FPMAS::api::scheduler::Epoch {
	public:
		MOCK_METHOD(void, submit, (FPMAS::api::scheduler::Job*), (override));
		MOCK_METHOD(const std::vector<FPMAS::api::scheduler::Job*>&, jobs, (), (const, override));
		MOCK_METHOD(iterator, begin, (), (override));
		MOCK_METHOD(iterator, end, (), (override));
		MOCK_METHOD(size_t, jobCount, (), (override));
		MOCK_METHOD(void, clear, (), (override));
};

#endif