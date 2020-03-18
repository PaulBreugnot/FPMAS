#ifndef GRID_H
#define GRID_H

#include "cell.h"
#include "environment.h"
#include "agent/agent.h"
#include "neigborhood.h"

using FPMAS::graph::base::LayerId;
using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
	static constexpr LayerId locationLayer(int N, int Range) {
		return N+Range;
	}
	// T = agent
	template<
		template<typename, typename, int> class Neighborhood = VonNeumann,
		int Range = 1,
		SYNC_MODE = GhostMode,
		int N = 1, // User provided layers
		typename... AgentTypes> class Grid
			: public Environment<S, N+Range+1, Cell<S, N+Range+1, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, N+Range+1, Cell<S, N+Range+1, AgentTypes...>, AgentTypes...> env_type;
					typedef Grid<Neighborhood, Range, S, N, AgentTypes...> grid_type;
					typedef Cell<S, N+Range+1, AgentTypes...> cell_type;

				private:
					const int _width;
					const int _height;
					Neighborhood<grid_type, cell_type, Range> neighborhood;

				public:
					static constexpr int Neighbor_Layer(int d) {
						return N+d-1;
					}
					static constexpr int locationLayer = N+Range;

					constexpr NodeId id(int x, int y) {
						return y * _width + x;
					};
					Grid(int width, int height);

					const int width() const {
						return _width;
					}
					const int height() const {
						return _height;
					}

			};

	template<template<typename, typename, int> class Neighborhood, int Range, SYNC_MODE, int N, typename... AgentTypes>
		Grid<Neighborhood, Range, S, N, AgentTypes...>::Grid(int width, int height): _width(width), _height(height), neighborhood(*this) {
			for(int i = 0; i < _width; i++) {
				for (int j = 0; j < _height; j++) {
					// Build cell
					this->buildNode(
							id(i, j),
							std::unique_ptr<typename env_type::agent_type>(new cell_type(i, j))
							);
				}
			}
			int arcId = 0;
			for(auto node : this->getNodes()) {
				const cell_type& cell = dynamic_cast<cell_type&>(*node.second->data()->read());
				neighborhood.linkNeighbors(node.first, cell);
			}
		}

}

namespace nlohmann {
	using FPMAS::environment::grid::Cell;
	template <SYNC_MODE, int N, typename... AgentTypes>
		struct adl_serializer<Cell<S, N, AgentTypes...>> {
			static void to_json(json& j, const Cell<S, N, AgentTypes...>& value) {
				j["x"] = value.x();
				j["y"] = value.y();
			}

			static Cell<S, N, AgentTypes...> from_json(const json& j) {
				return Cell<S, N, AgentTypes...>(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
