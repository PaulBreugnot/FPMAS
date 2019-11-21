#include "config.h"

namespace FPMAS {
	namespace config {
		void zoltan_config(Zoltan* zz) {
			zz->Set_Param("DEBUG_LEVEL", "0");
			zz->Set_Param("LB_METHOD", "GRAPH"); // Simulation parameter?
			zz->Set_Param("LB_APPROACH", "REPARTITION"); //REPARTITION //REFINE //PARTITION
			zz->Set_Param("NUM_GID_ENTRIES", "2");
			zz->Set_Param("NUM_LID_ENTRIES", "0");
			zz->Set_Param("OBJ_WEIGHT_DIM", "1");
			zz->Set_Param("EDGE_WEIGHT_DIM", "1");
			zz->Set_Param("RETURN_LISTS", "ALL");
			zz->Set_Param("CHECK_GRAPH", "0");
			zz->Set_Param("IMBALANCE_TOL", "1.02");
			zz->Set_Param("PHG_EDGE_SIZE_THRESHOLD", "1.0");
		}

	}
}
