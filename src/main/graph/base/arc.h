#ifndef ARC_H
#define ARC_H

#include <nlohmann/json.hpp>

#include <array>
#include "graph_item.h"
#include "graph.h"
#include "node.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<NODE_PARAMS> class Graph;

	/**
	 * Graph arc.
	 *
	 * @tparam T associated data type
	 */
	template<
		class T,
		typename LayerType = DefaultLayer,
		int N = 1
		>
	class Arc : public GraphItem {
		// Grants access to the Arc constructor
		friend NODE;
		friend Layer<NODE_PARAMS_SPEC>;
		friend Arc<NODE_PARAMS_SPEC>* Graph<NODE_PARAMS_SPEC>::link(NODE*, NODE*, unsigned long, LayerType);
		friend Arc<NODE_PARAMS_SPEC> nlohmann::adl_serializer<Arc<NODE_PARAMS_SPEC>>::from_json(const json&);

		protected:
		Arc(unsigned long, NODE*, NODE*);
		Arc(unsigned long, NODE*, NODE*, LayerType layer);
		/**
		 * Pointer to source Node.
		 */
		NODE* sourceNode;
		/**
		 * Pointer to target Node.
		 */
		NODE* targetNode;

		public:
		const LayerType layer;
		NODE* getSourceNode() const;
		NODE* getTargetNode() const;

	};

	/**
	 * Private arc constructor.
	 *
	 * @param id arc id
	 * @param sourceNode pointer to the source Node
	 * @param targetNode pointer to the target Node
	 */
	template<NODE_PARAMS> Arc<NODE_PARAMS_SPEC>::Arc(unsigned long id, NODE* sourceNode, NODE* targetNode, LayerType layer)
		: GraphItem(id), sourceNode(sourceNode), targetNode(targetNode), layer(layer) {
			this->sourceNode->layer(layer).outgoingArcs.push_back(this);
			this->targetNode->layer(layer).incomingArcs.push_back(this);
		};

	template<NODE_PARAMS> Arc<NODE_PARAMS_SPEC>::Arc(unsigned long id, NODE* sourceNode, NODE* targetNode) : Arc(id, sourceNode, targetNode, LayerType(0)) {

	}
	/**
	 * Returns the source node of this arc.
	 *
	 * @return pointer to the source node
	 */
	template<NODE_PARAMS> NODE* Arc<NODE_PARAMS_SPEC>::getSourceNode() const {
		return this->sourceNode;
	}

	/**
	 * Returns the target node of this arc.
	 *
	 * @return pointer to the target node
	 */
	template<NODE_PARAMS> NODE* Arc<NODE_PARAMS_SPEC>::getTargetNode() const {
		return this->targetNode;
	}
}

#endif
