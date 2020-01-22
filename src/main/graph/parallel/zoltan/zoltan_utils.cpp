#include "zoltan_utils.h"

namespace FPMAS::graph::parallel::zoltan::utils {
	/**
	 * Convenient function to rebuild a regular node or arc id, as an
	 * unsigned long, from a ZOLTAN_ID_PTR global id array, that actually
	 * stores 2 unsigned int for each node unsigned long id.
	 * So, with our configuration, we use 2 unsigned int in Zoltan to
	 * represent each id. The `i` input parameter should correspond to the
	 * first part index of the node to query in the `global_ids` array. In
	 * consequence, `i` should actually always be even.
	 *
	 * @param global_ids an adress to a pair of Zoltan global ids
	 * @return rebuilt node id
	 */
	// Note : Zoltan should be able to directly use unsigned long as ids.
	// However, this require to set up compile time flags to Zoltan, so we
	// use 2 entries of the default zoltan unsigned int instead, to make it
	// compatible with any Zoltan installation.
	// Passing compile flags to the custom embedded CMake Zoltan
	// installation might also be possible.
	unsigned long read_zoltan_id(const ZOLTAN_ID_PTR global_ids) {
		return ((unsigned long) (global_ids[0] << 16)) | global_ids[1];
	}

	/**
	 * Writes zoltan global ids to the specified `global_ids` adress.
	 *
	 * The original `unsigned long` is decomposed into 2 `unsigned int` to
	 * fit the default Zoltan data structure. The written id can then be
	 * rebuilt using the node_id(const ZOLTAN_ID_PTR) function.
	 *
	 * @param id id to write
	 * @param global_ids adress to a Zoltan global_ids array
	 */
	void write_zoltan_id(unsigned long id, ZOLTAN_ID_PTR global_ids) {
		global_ids[0] = (id & 0xFFFF0000) >> 16 ;
		global_ids[1] = (id & 0xFFFF);
	}


}