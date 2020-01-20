#include "gtest/gtest.h"
#include "mpi.h"
#include "zoltan_cpp.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);

	MPI_Init(&argc, &argv);
	float v;
	Zoltan_Initialize(argc, argv, &v);

	int result;
	result =  RUN_ALL_TESTS();

	MPI_Finalize();
	return result;
}
