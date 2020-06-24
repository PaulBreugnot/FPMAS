#include "runtime.h"

namespace fpmas::runtime {

	void Runtime::run(Date start, Date end) {
		for(Date time = start; time < end; time++) {
			date = time;
			scheduler.build(time, epoch);
			for(const api::scheduler::Job* job : epoch) {
				job->getBeginTask().run();
				for(api::scheduler::Task* task : *job) {
					task->run();
				}
				job->getEndTask().run();
			}
		}
	}

	void Runtime::run(Date end) {
		run(0, end);
	}
}