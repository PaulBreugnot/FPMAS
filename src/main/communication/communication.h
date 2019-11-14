#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <mpi.h>

namespace FPMAS {
	namespace communication {
		class MpiCommunicator {
			private:
				int size;
				int rank;

				MPI_Group group;
				MPI_Comm comm;

			public:
				MpiCommunicator();
				MPI_Group getMpiGroup();
				MPI_Comm getMpiComm();

				int getRank();
				int getSize();

		};
	}
}
#endif
