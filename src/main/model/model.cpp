#include "model.h"

namespace FPMAS::model {

	const JID Model::LB_JID = 0;

	AgentGroup::AgentGroup(AgentGraph& agent_graph, JID job_id)
		: agent_graph(agent_graph), _job(job_id), sync_graph_task(agent_graph) {
			_job.setEndTask(sync_graph_task);
	}

	void AgentGroup::add(api::model::Agent* agent) {
		_agents.push_back(agent);
		_job.add(*new AgentTask(*agent));
	}

	Model::Model(AgentGraph& graph, LoadBalancingAlgorithm& load_balancing)
		: _graph(graph), _runtime(_scheduler), _loadBalancingJob(LB_JID), load_balancing_task(_graph, load_balancing, _scheduler, _runtime) {
			_loadBalancingJob.add(load_balancing_task);
		}

	Model::~Model() {
		for(auto group : _groups)
			delete group;
	}

	AgentGroup* Model::buildGroup() {
		AgentGroup* group = new AgentGroup(_graph, job_id++);
		_groups.push_back(group); 
		return group;
	}

	void LoadBalancingTask::run() {
		scheduler::Epoch epoch;
		scheduler.build(runtime.currentDate() + 1, epoch);
		ConstNodeMap node_map;
		PartitionMap fixed_nodes;
		PartitionMap partition;
		for(const api::scheduler::Job* job : epoch) {
			for(api::scheduler::Task* task : *job) {
				if(AgentTask* agent_task = dynamic_cast<AgentTask*>(task)) {
					auto node = agent_task->agent().node();
					node_map[node->getId()] = node;
				}
			}
			partition = load_balancing.balance(node_map, fixed_nodes);
			fixed_nodes = partition;
		};
		agent_graph.distribute(partition);
	};
}
