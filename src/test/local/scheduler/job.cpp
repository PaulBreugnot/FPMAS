#include "scheduler/scheduler.h"

#include "../mocks/scheduler/mock_scheduler.h"

using FPMAS::scheduler::Job;

using ::testing::Ref;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;

class JobTest : public ::testing::Test {
	protected:
		const FPMAS::JID id = 236;
		Job job {id};
};

TEST_F(JobTest, id) {
	ASSERT_EQ(job.id(), id);
}

TEST_F(JobTest, add) {
	std::array<MockTask, 6> tasks;
	
	for(auto& task : tasks)
		job.add(task);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[3], &tasks[4], &tasks[5]
				));
}

TEST_F(JobTest, begin) {
	MockTask begin;

	job.setBeginTask(begin);
	ASSERT_THAT(job.getBeginTask(), Ref(begin));

	// Begin should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}

TEST_F(JobTest, end) {
	MockTask end;

	job.setEndTask(end);
	ASSERT_THAT(job.getEndTask(), Ref(end));

	// End should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}
