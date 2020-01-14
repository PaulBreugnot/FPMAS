#include <thread>
#include <chrono>

#include "gtest/gtest.h"
#include "communication/communication.h"

using FPMAS::communication::MpiCommunicator;

TEST(Mpi_MpiCommunicatorTest, size_test) {
	MpiCommunicator comm;

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	ASSERT_EQ(size, comm.getSize());
}

TEST(Mpi_MpiCommunicatorTest, rank_test) {
	MpiCommunicator comm;

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	ASSERT_EQ(rank, comm.getRank());
}

TEST(Mpi_MpiCommunicatorTest, build_from_ranks_test) {
	int global_size;
	MPI_Comm_size(MPI_COMM_WORLD, &global_size);

	if(global_size == 1) {
		MpiCommunicator comm {0};
		int comm_size;
		MPI_Comm_size(comm.getMpiComm(), &comm_size);
		ASSERT_EQ(comm.getSize(), 1);
		ASSERT_EQ(comm.getSize(), comm_size);

		int comm_rank;
		MPI_Comm_rank(comm.getMpiComm(), &comm_rank);
		ASSERT_EQ(comm.getRank(), 0);
		ASSERT_EQ(comm.getRank(), comm_rank);
	}
	else if(global_size >= 2) {
		int current_rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);

		if(!(global_size % 2 == 1 && current_rank == (global_size-1))) {
			MpiCommunicator* comm;
			if(current_rank % 2 == 0) {
				comm = new MpiCommunicator {current_rank, (current_rank + 1) % global_size};
			}
			else {
				comm = new MpiCommunicator {(current_rank - 1) % global_size, current_rank};
			}

			int comm_size;
			MPI_Comm_size(comm->getMpiComm(), &comm_size);
			ASSERT_EQ(comm->getSize(), 2);
			ASSERT_EQ(comm->getSize(), comm_size);

			int comm_rank;
			MPI_Comm_rank(comm->getMpiComm(), &comm_rank);
			ASSERT_EQ(comm->getRank(), comm_rank);
			delete comm;
		}
		else {
			// MPI requirement : all processes must call MPI_Comm_create
			MpiCommunicator comm {global_size - 1};
		}
	}

}

TEST(Mpi_TerminationTest, simple_termination_test) {
	MpiCommunicator comm;

	comm.terminate();

}

using namespace std::chrono_literals;

TEST(Mpi_TerminationTest, termination_test_with_delay) {
	MpiCommunicator comm;

	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		std::this_thread::sleep_for(1s);
	}

	comm.terminate();
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);

}
