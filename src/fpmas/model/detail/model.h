#ifndef FPMAS_MODEL_DETAIL_H
#define FPMAS_MODEL_DETAIL_H

/** \file src/fpmas/model/detail/model.h
 * Model implementation details.
 */

#include "fpmas/api/model/model.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/zoltan_load_balancing.h"
#include "fpmas/graph/scheduled_load_balancing.h"
#include "fpmas/runtime/runtime.h"

namespace fpmas {
	namespace model {
		using api::model::AgentNode;
		using api::model::AgentEdge;
		using api::model::AgentPtr;
		using api::scheduler::JID;

		/**
		 * fpmas::model implementation details.
		 */
		namespace detail {
			/**
			 * api::model::AgentTask implementation.
			 */
			class AgentTask : public api::model::AgentTask {
				private:
					api::model::AgentPtr& _agent;
				public:
					/**
					 * AgentTask constructor.
					 *
					 * @param agent_ptr agents that will be executed by this task
					 */
					AgentTask(api::model::AgentPtr& agent_ptr)
						: _agent(agent_ptr) {}

					const api::model::AgentPtr& agent() const override {
						return _agent;
					}

					AgentNode* node() override
					{return _agent->node();}

					void run() override {
						_agent->act();
					}
			};

			/**
			 * Graph synchronization task.
			 *
			 * This task is set as the end task of each \AgentGroup's job.
			 *
			 * Concretely, this means that the simulation Graph is synchronized at the
			 * end of each \AgentGroup execution.
			 */
			class SynchronizeGraphTask : public api::scheduler::Task {
				private:
					api::model::AgentGraph& agent_graph;
				public:
					/**
					 * SynchronizeGraphTask constructor.
					 *
					 * @param agent_graph Agent graph to synchronize
					 */
					SynchronizeGraphTask(api::model::AgentGraph& agent_graph)
						: agent_graph(agent_graph) {}

					/**
					 * Calls api::model::AgentGraph::synchronize().
					 */
					void run() override {
						agent_graph.synchronize();
					}
			};

			class EraseAgentCallback;

			/**
			 * Callback triggered when an \AgentNode is _inserted_ into the simulation
			 * graph (i.e. when an \AgentNode is created, or when it's imported from an
			 * other process).
			 */
			class InsertAgentNodeCallback 
				: public api::utils::Callback<AgentNode*> {
					private:
						api::model::Model& model;
					public:
						/**
						 * InsertAgentNodeCallback constructor.
						 *
						 * @param model current model
						 */
						InsertAgentNodeCallback(api::model::Model& model) : model(model) {}

						/**
						 * When called, the argument is assumed to contain a new
						 * uninitialized \Agent (accessible through node->data()).
						 * The \Agent GroupId is also assumed to be initialized.
						 *
						 * Then this \Agent is bound to :
						 * - its corresponding group (api::model::Agent::setGroup(),
						 *   api::model::AgentGroup::insert()). The group is
						 *   retrieved from the current model using
						 *   api::model::Model::getGroup().
						 * - the argument node (api::model::Agent::setNode)
						 * - the current simulation graph
						 *   (api::model::Agent::setGraph())
						 * - a new AgentTask (api::model::Agent::setTask())
						 *
						 * All the corresponding fields are valid once this callback
						 * has been triggered.
						 *
						 * @param node \AgentNode inserted in the graph
						 */
						void call(api::model::AgentNode* node) override;
				};

			/**
			 * Callback triggered when an \AgentNode is _erased_ from the simulation
			 * graph (i.e. when an \AgentNode is created, or when its imported from an
			 * other process).
			 */
			class EraseAgentNodeCallback 
				: public api::utils::Callback<AgentNode*> {
					private:
						api::model::Model& model;
					public:
						/**
						 * EraseAgentNodeCallback constructor.
						 *
						 * @param model current model
						 */
						EraseAgentNodeCallback(api::model::Model& model) : model(model) {}

						/**
						 * Removes the node's \Agent (i.e. node->data()) from its
						 * \AgentGroup.
						 *
						 * If the node was still \LOCAL, the agent's task is also
						 * unscheduled.
						 *
						 * @param node \AgentNode to erase from the graph
						 */
						void call(api::model::AgentNode* node) override;
				};

			/**
			 * Callback triggered when an \AgentNode is set \LOCAL.
			 *
			 * This happens when a previously \DISTANT node is imported into the local
			 * graph and becomes \LOCAL, or when a new \AgentNode is inserted in the
			 * graph (the node is implicitly set \LOCAL in this case).
			 */
			class SetAgentLocalCallback
				: public api::utils::Callback<AgentNode*> {
					private:
						api::model::Model& model;
					public:
						/**
						 * SetAgentLocalCallback constructor.
						 *
						 * @param model current model
						 */
						SetAgentLocalCallback(api::model::Model& model) : model(model) {}

						/**
						 * Schedules the associated agent's task to be executed within
						 * its \AgentGroup's job.
						 *
						 * @param node \AgentNode to set \LOCAL
						 *
						 * @see \AgentTask
						 * @see fpmas::api::model::Agent::task()
						 * @see fpmas::api::model::AgentGroup::job()
						 */
						void call(api::model::AgentNode* node) override;
				};

			/**
			 * Callback triggered when an \AgentNode is set \DISTANT.
			 *
			 * This happens when a previously \LOCAL node is exported to an other
			 * process, or when a \DISTANT node is inserted into the graph upon edge
			 * import.
			 */
			class SetAgentDistantCallback
				: public api::utils::Callback<AgentNode*> {
					private:
						api::model::Model& model;
					public:
						/**
						 * SetAgentDistantCallback constructor.
						 *
						 * @param model current model
						 */
						SetAgentDistantCallback(api::model::Model& model) : model(model) {}

						/**
						 * Unschedules the associated agent's task.
						 *
						 * Notice that when a node goes \DISTANT, it is still contained
						 * in the simulation graph as any other node. However, it
						 * **must not** be executed, since only \LOCAL agents are
						 * assumed to be executed by the local process.
						 *
						 * @param node \AgentNode to set \DISTANT
						 *
						 * @see \AgentTask
						 * @see fpmas::api::model::Agent::task()
						 * @see fpmas::api::model::AgentGroup::job()
						 */
						void call(api::model::AgentNode* node) override;
				};

			/**
			 * Partial graph::DistributedGraph specialization used as the Agent
			 * simulation graph.
			 */
			template<template<typename> class SyncMode>
				using AgentGraph = graph::DistributedGraph<AgentPtr, SyncMode>;

			/**
			 * ZoltanLoadBalancing specialization.
			 */
			typedef graph::ZoltanLoadBalancing<AgentPtr> ZoltanLoadBalancing;
			/**
			 * ScheduledLoadBalancing specialization.
			 */
			typedef graph::ScheduledLoadBalancing<AgentPtr> ScheduledLoadBalancing;

			/**
			 * Load balancing task.
			 *
			 * This task is actually the unique task of the \Job defined by
			 * Model::loadBalancingJob().
			 *
			 * @see api::model::AgentGraph::balance()
			 */
			class LoadBalancingTask : public api::scheduler::Task {
				public:
					/**
					 * LoadBalancingAlgorithm type.
					 */
					typedef api::graph::LoadBalancing<AgentPtr>
						LoadBalancingAlgorithm;
					/**
					 * Agent node map.
					 */
					typedef api::graph::NodeMap<AgentPtr> NodeMap;
					/**
					 * Partition map.
					 */
					typedef typename api::graph::PartitionMap PartitionMap;

				private:
					api::model::AgentGraph& agent_graph;
					LoadBalancingAlgorithm& load_balancing;

				public:
					/**
					 * LoadBalancingTask constructor.
					 *
					 * @param agent_graph associated agent graph on which load
					 * balancing will be performed
					 * @param load_balancing load balancing algorithme used to compute
					 * a balanced partition
					 */
					LoadBalancingTask(
							api::model::AgentGraph& agent_graph,
							LoadBalancingAlgorithm& load_balancing
							) : agent_graph(agent_graph), load_balancing(load_balancing) {}

					void run() override;
			};

			/**
			 * api::model::AgentGroup implementation.
			 */
			class AgentGroup : public api::model::AgentGroup {
				friend detail::EraseAgentCallback;
				private:
				api::model::GroupId id;
				api::model::AgentGraph& agent_graph;
				scheduler::Job _job;
				detail::SynchronizeGraphTask sync_graph_task;
				std::vector<api::model::AgentPtr*> _agents;

				public:
				/**
				 * AgentGroup constructor.
				 *
				 * @param group_id unique id of the group
				 * @param agent_graph associated agent graph
				 */
				AgentGroup(api::model::GroupId group_id, api::model::AgentGraph& agent_graph);

				AgentGroup& operator=(const AgentGroup&) = delete;
				AgentGroup& operator=(AgentGroup&&) = delete;

				api::model::GroupId groupId() const override {return id;}

				void add(api::model::Agent*) override;
				void remove(api::model::Agent*) override;

				void insert(api::model::AgentPtr*) override;
				void erase(api::model::AgentPtr*) override;

				scheduler::Job& job() override {return _job;}
				const scheduler::Job& job() const override {return _job;}
				std::vector<api::model::Agent*> agents() const override;
				std::vector<api::model::Agent*> localAgents() const override;
			};


			/**
			 * api::model::Model implementation.
			 */
			class Model : public api::model::Model {
				public:
					/**
					 * LoadBalancingAlgorithm type.
					 */
					typedef typename LoadBalancingTask::LoadBalancingAlgorithm LoadBalancingAlgorithm;

				private:
					api::model::AgentGraph& _graph;
					api::scheduler::Scheduler& _scheduler;
					api::runtime::Runtime& _runtime;
					scheduler::Job _loadBalancingJob;
					LoadBalancingTask load_balancing_task;
					InsertAgentNodeCallback* insert_node_callback = new InsertAgentNodeCallback(*this);
					EraseAgentNodeCallback* erase_node_callback = new EraseAgentNodeCallback(*this);
					SetAgentLocalCallback* set_local_callback = new SetAgentLocalCallback(*this);
					SetAgentDistantCallback* set_distant_callback = new SetAgentDistantCallback(*this);

					std::unordered_map<api::model::GroupId, api::model::AgentGroup*> _groups;

				public:
					/**
					 * Model constructor.
					 *
					 * @param graph simulation graph
					 * @param scheduler scheduler
					 * @param runtime runtime
					 * @param load_balancing load balancing algorithm
					 */
					Model(
							api::model::AgentGraph& graph,
							api::scheduler::Scheduler& scheduler,
							api::runtime::Runtime& runtime,
							LoadBalancingAlgorithm& load_balancing);
					Model(const Model&) = delete;
					Model(Model&&) = delete;
					Model& operator=(const Model&) = delete;
					Model& operator=(Model&&) = delete;

					api::model::AgentGraph& graph() override {return _graph;}
					api::scheduler::Scheduler& scheduler() override {return _scheduler;}
					api::runtime::Runtime& runtime() override {return _runtime;}

					const scheduler::Job& loadBalancingJob() const override {return _loadBalancingJob;}

					api::model::AgentGroup& buildGroup(api::model::GroupId id) override;
					api::model::AgentGroup& getGroup(api::model::GroupId id) const override;
					const std::unordered_map<api::model::GroupId, api::model::AgentGroup*>& groups() const override {return _groups;}

					AgentEdge* link(api::model::Agent* src_agent, api::model::Agent* tgt_agent, api::graph::LayerId layer) override;
					void unlink(api::model::AgentEdge* edge) override;

					~Model();
			};

			/**
			 * A default Model configuration that instantiate default components
			 * implementations.
			 */
			template<template<typename> class SyncMode>
				class DefaultModelConfig {
					protected:
						/**
						 * Default MpiCommunicator.
						 */
						communication::MpiCommunicator comm;
						/**
						 * Default AgentGraph.
						 */
						AgentGraph<SyncMode> __graph {comm};
						/**
						 * Default Scheduler.
						 */
						scheduler::Scheduler __scheduler;
						/**
						 * Default Runtime.
						 */
						runtime::Runtime __runtime {__scheduler};
						/**
						 * Default load balancing algorithm.
						 */
						ZoltanLoadBalancing __zoltan_lb {comm};
						/**
						 * Default scheduled load balancing algorithm.
						 */
						ScheduledLoadBalancing __load_balancing {__zoltan_lb, __scheduler, __runtime};
				};

			/**
			 * Model extension based on the DefaultModelConfig.
			 *
			 * It is recommended to use this helper class to instantiate a Model.
			 *
			 * @par Example
			 * ```cpp
			 * // main.cpp
			 * #include "fpmas.h"
			 *
			 * using namespace fpmas;
			 *
			 * class UserAgent1 : public model::AgentBase<UserAgent1> {
			 * 	public:
			 * 	void act() override {
			 * 		FPMAS_LOG_INFO(UserAgent1, "Execute agent %s",
			 * 		FPMAS_C_STR(this->node()->getId()))
			 * 	}
			 * };
			 *
			 * class UserAgent2 : public model::AgentBase<UserAgent2> {
			 * 	public:
			 * 	void act() override {
			 * 		FPMAS_LOG_INFO(UserAgent2, "Execute agent %s",
			 * 		FPMAS_C_STR(this->node()->getId()))
			 * 	}
			 * };
			 *
			 * #define USER_AGENTS UserAgent1, UserAgent2
			 *
			 * FPMAS_DEFAULT_JSON(UserAgent1)
			 * FPMAS_DEFAULT_JSON(UserAgent2)
			 *
			 * FPMAS_JSON_SET_UP(USER_AGENTS)
			 *
			 *
			 * int main(int argc, char** argv) {
			 * 	fpmas::init(argc, argv);
			 * 	FPMAS_REGISTER_AGENT_TYPES(USER_AGENTS);
			 *
			 * 	{
			 * 		model::DefaultModel model;
			 *
			 * 		model::AgentGroup& group_1 = model.buildGroup();
			 * 		group_1.add(new UserAgent1);
			 * 		group_1.add(new UserAgent2);
			 *
			 * 		model.scheduler().schedule(0, 1, group_1.job());
			 * 		model.scheduler().schedule(0, 50, model.loadBalancingJob());
			 *
			 * 		model.runtime().run(1000);
			 * 	}
			 * 	fpmas::finalize();
			 * }
			 * ```
			 */
			template<template<typename> class SyncMode>
				class DefaultModel : private DefaultModelConfig<SyncMode>, public Model {
					public:
						DefaultModel() :
							DefaultModelConfig<SyncMode>(),
							Model(this->__graph, this->__scheduler, this->__runtime, this->__load_balancing) {}

						/**
						 * Returns a reference to the internal
						 * communication::MpiCommunicator.
						 *
						 * @return reference to internal MPI communicator
						 */
						communication::MpiCommunicator& getMpiCommunicator() {
							return this->comm;
						}

						/**
						 * \copydoc getMpiCommunicator
						 */
						const communication::MpiCommunicator& getMpiCommunicator() const {
							return this->comm;
						}
				};

		}
	}
}
#endif