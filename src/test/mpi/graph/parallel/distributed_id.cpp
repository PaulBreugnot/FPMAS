#include "gtest/gtest.h"

#include "utils/macros.h"
#include "graph/parallel/distributed_id.h"

TEST(Mpi_DistributedId, test_mpi_send_distributed_id) {
	MpiDistributedId id;
	id.rank = 10;
	id.id = 2;

	int current_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
	MPI_Request req;
	MPI_Isend(&id, 1, MPI_DISTRIBUTED_ID_TYPE, current_rank, 0, MPI_COMM_WORLD, &req);
	MPI_Status status;

	MpiDistributedId recvId;
	MPI_Recv(&recvId, 1, MPI_DISTRIBUTED_ID_TYPE, current_rank, 0, MPI_COMM_WORLD, &status);
	MPI_Wait(&req, &status);

	ASSERT_EQ(recvId.rank, 10);
	ASSERT_EQ(recvId.id, 2);
}