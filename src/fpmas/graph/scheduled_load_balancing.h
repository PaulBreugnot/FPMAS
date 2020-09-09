#ifndef FPMAS_SCHEDULED_LOAD_BALANCING_H
#define FPMAS_SCHEDULED_LOAD_BALANCING_H

/** \file src/fpmas/graph/scheduled_load_balancing.h
 * ScheduledLoadBalancing implementation.
 */

#include "fpmas/api/graph/graph.h"
#include "fpmas/api/runtime/runtime.h"
#include "fpmas/scheduler/scheduler.h"

namespace fpmas { namespace graph {
	using api::graph::PartitionMap;
	using api::graph::NodeMap;

	/**
	 * api::graph::LoadBalancing implementation that takes nodes' task
	 * scheduling into account.
	 */
	template<typename T>
		class ScheduledLoadBalancing : public api::graph::LoadBalancing<T> {
			private:
				api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb;
				api::scheduler::Scheduler& scheduler;
				api::runtime::Runtime& runtime;

			public:
				/**
				 * ScheduledLoadBalancing constructor.
				 *
				 * @param fixed_vertices_lb fixed vertices load balancing
				 * algorithm
				 * @param scheduler current scheduler (used to access scheduled
				 * tasks associated to nodes)
				 * @param runtime current runtime (used to access the current
				 * api::runtime::Runtime::currentDate())
				 */
				ScheduledLoadBalancing<T>(
						api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb,
						api::scheduler::Scheduler& scheduler,
						api::runtime::Runtime& runtime
						) : fixed_vertices_lb(fixed_vertices_lb), scheduler(scheduler), runtime(runtime) {}

				PartitionMap balance(NodeMap<T> nodes) override;

		};

	template<typename T>
		PartitionMap ScheduledLoadBalancing<T>::balance(NodeMap<T> nodes) {
			scheduler::Epoch epoch;
			scheduler.build(runtime.currentDate() + 1, epoch);
			NodeMap<T> node_map;
			PartitionMap fixed_nodes;
			PartitionMap partition;
			for(const api::scheduler::Job* job : epoch) {
				for(api::scheduler::Task* task : *job) {
					if(api::scheduler::NodeTask<T>* node_task = dynamic_cast<api::scheduler::NodeTask<T>*>(task)) {
						auto node = node_task->node();
						node_map[node->getId()] = node;
					}
				}
				partition = fixed_vertices_lb.balance(node_map, fixed_nodes);
				fixed_nodes = partition;
			};
			partition = fixed_vertices_lb.balance(nodes, fixed_nodes);
			return partition;
		}
}}
#endif