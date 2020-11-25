#ifndef FPMAS_GRID_API_H
#define FPMAS_GRID_API_H

#include "spatial_model.h"

namespace fpmas { namespace api { namespace model {

	typedef long DiscreteCoordinate;

	struct DiscretePoint {
		DiscreteCoordinate x;
		DiscreteCoordinate y;

		DiscretePoint() {}
		DiscretePoint(DiscreteCoordinate x, DiscreteCoordinate y)
			: x(x), y(y) {}
	};

	class GridCell : public virtual Cell {
		public:
			virtual DiscretePoint location() const = 0;
	};

	class GridAgent : public virtual SpatialAgent {
		protected:
			using SpatialAgent::moveTo;

			virtual void moveTo(DiscretePoint point) = 0;
		public:
			virtual DiscretePoint locationPoint() const = 0;
	};

	class GridCellFactory {
		public:
			virtual GridCell* build(DiscretePoint location) = 0;

			virtual ~GridCellFactory() {}
	};
}}}

namespace fpmas {
	std::string to_string(const api::model::DiscretePoint& point);

	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const DiscretePoint& point);
	}}
}
#endif
