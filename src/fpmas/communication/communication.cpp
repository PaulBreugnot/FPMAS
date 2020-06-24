#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

#include "fpmas/utils/macros.h"

namespace fpmas::communication {
	/**
	 * Builds an MPI group and the associated communicator as a copy of the
	 * MPI_COMM_WORLD communicator.
	 */
	MpiCommunicator::MpiCommunicator() {
		MPI_Group worldGroup;
		MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
		MPI_Group_union(worldGroup, MPI_GROUP_EMPTY, &this->group);
		MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

		MPI_Group_free(&worldGroup);
	}

	/**
	 * Builds an MPI group and the associated communicator that contains the ranks
	 * specified as arguments.
	 *
	 * ```cpp
	 * fpmas::communication::MpiCommunicator comm {0, 1, 2};
	 * ```
	 *
	 * Notice that such calls must follow the same requirements as the
	 * `MPI_Comm_create` function. In consequence, **all the current procs** must
	 * perform a call to this function to be valid.
	 *
	 * ```cpp
	 * int current_ranks;
	 * MPI_Comm_rank(&current_rank);
	 *
	 * if(current_rank < 3) {
	 * 	MpiCommunicator comm {0, 1, 2};
	 * } else {
	 * 	MpiCommunicator comm {current_rank};
	 * }
	 * ```
	 *
	 * @param ranks ranks to include in the group / communicator
	 */
	MpiCommunicator::MpiCommunicator(std::initializer_list<int> ranks) {
		int* _ranks = (int*) std::malloc(ranks.size()*sizeof(int));
		int i = 0;
		for(auto rank : ranks) {
			_ranks[i++] = rank;
		}

		MPI_Group worldGroup;
		MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
		MPI_Group_incl(worldGroup, ranks.size(), _ranks, &this->group);
		MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

		MPI_Group_free(&worldGroup);

		std::free(_ranks);
	}

	/**
	 * fpmas::communication::MpiCommunicator copy constructor.
	 *
	 * Allows a correct MPI resources management.
	 *
	 * @param from fpmas::communication::MpiCommunicator to copy from
	 */
	MpiCommunicator::MpiCommunicator(const fpmas::communication::MpiCommunicator& from) {
		this->comm = from.comm;
		this->comm_references = from.comm_references + 1;

		MPI_Comm_group(this->comm, &this->group);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

	}
	/**
	 * fpmas::communication::MpiCommunicator copy assignment.
	 *
	 * Allows a correct MPI resources management.
	 *
	 * @param other fpmas::communication::MpiCommunicator to assign to this
	 */
	MpiCommunicator& fpmas::communication::MpiCommunicator::operator=(const MpiCommunicator& other) {
		MPI_Group_free(&this->group);

		this->comm = other.comm;
		this->comm_references = other.comm_references;

		MPI_Comm_group(this->comm, &this->group);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);


		return *this;
	}

	/**
	 * Returns the built MPI communicator.
	 *
	 * @return associated MPI communicator
	 */
	MPI_Comm MpiCommunicator::getMpiComm() const {
		return this->comm;
	}

	/**
	 * Returns the built MPI group.
	 *
	 * @return associated MPI group
	 */
	MPI_Group MpiCommunicator::getMpiGroup() const {
		return this->group;
	}

	/**
	 * Returns the rank of this communicator, in the meaning of MPI communicator
	 * ranks.
	 *
	 * @return MPI communicator rank
	 */
	int MpiCommunicator::getRank() const {
		return this->rank;
	}

	/**
	 * Returns the rank of this communicator, in the meaning of MPI communicator
	 * sizes.
	 *
	 * @return MPI communicator size
	 */
	int MpiCommunicator::getSize() const {
		return this->size;
	}

	void MpiCommunicator::send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) {
		MPI_Send(data, count, datatype, destination, tag, this->comm);
	}

	void MpiCommunicator::send(int destination, int tag) {
		MPI_Send(NULL, 0, MPI_CHAR, destination, tag, this->comm);
	}

	void MpiCommunicator::Issend(
			const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) {
		MPI_Issend(data, count, datatype, destination, tag, this->comm, req);
	}

	void MpiCommunicator::Issend(int destination, int tag, MPI_Request* req) {
		MPI_Issend(NULL, 0, MPI_CHAR, destination, tag, this->comm, req);
	}

	void MpiCommunicator::recv(MPI_Status* status) {
		MPI_Recv(NULL, 0, MPI_INT, status->MPI_SOURCE, status->MPI_TAG, this->comm, status);
	}

	void MpiCommunicator::recv(
		void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) {
		MPI_Recv(buffer, count, datatype, source, tag, this->comm, status);
	}

	void MpiCommunicator::probe(int source, int tag, MPI_Status* status) {
		MPI_Probe(source, tag, this->comm, status);
	}

	bool MpiCommunicator::Iprobe(int source, int tag, MPI_Status* status) {
		int flag;
		MPI_Iprobe(source, tag, this->comm, &flag, status);
		return flag > 0;
	}

	bool MpiCommunicator::test(MPI_Request* req) {
		MPI_Status status;
		int flag;
		MPI_Test(req, &flag, &status);
		return flag > 0;
	}

	std::unordered_map<int, DataPack> 
		MpiCommunicator::allToAll (
				std::unordered_map<int, DataPack> 
				data_pack, MPI_Datatype datatype) {
			// Migrate
			int* sendcounts = (int*) std::malloc(getSize()*sizeof(int));
			int* sdispls = (int*) std::malloc(getSize()*sizeof(int));

			int* size_buffer = (int*) std::malloc(getSize()*sizeof(int));

			int current_sdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				sendcounts[i] = data_pack[i].count;
				sdispls[i] = current_sdispls;
				current_sdispls += sendcounts[i];

				size_buffer[i] = sendcounts[i];
			}
			
			// Sends size / displs to each rank, and receive recvs size / displs from
			// each rank.
			MPI_Alltoall(MPI_IN_PLACE, 0, MPI_INT, size_buffer, 1, MPI_INT, getMpiComm());

			int type_size;
			MPI_Type_size(datatype, &type_size);

			void* send_buffer = std::malloc(current_sdispls * type_size);
			for(int i = 0; i < getSize(); i++) {
				std::memcpy(&((char*) send_buffer)[sdispls[i]], data_pack[i].buffer, data_pack[i].size);
			}

			int* recvcounts = (int*) std::malloc(getSize()*sizeof(int));
			int* rdispls = (int*) std::malloc(getSize()*sizeof(int));
			int current_rdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				recvcounts[i] = size_buffer[i];
				rdispls[i] = current_rdispls;
				current_rdispls += recvcounts[i];
			}

			void* recv_buffer = std::malloc(current_rdispls * type_size);

			MPI_Alltoallv(
					send_buffer, sendcounts, sdispls, datatype,
					recv_buffer, recvcounts, rdispls, datatype,
					getMpiComm()
					);

			std::unordered_map<int, DataPack> imported_data_pack;
			for (int i = 0; i < getSize(); i++) {
				if(recvcounts[i] > 0) {
					imported_data_pack[i] = DataPack(recvcounts[i], type_size);
					DataPack& dataPack = imported_data_pack[i];

					std::memcpy(dataPack.buffer, &((char*) recv_buffer)[rdispls[i]], dataPack.size);
				}
			}

			std::free(sendcounts);
			std::free(sdispls);
			std::free(size_buffer);
			std::free(recvcounts);
			std::free(rdispls);
			std::free(send_buffer);
			std::free(recv_buffer);
			return imported_data_pack;
		}

	std::vector<DataPack> MpiCommunicator::gather(DataPack data, MPI_Datatype type, int root) {

			int type_size;
			MPI_Type_size(type, &type_size);


			void* send_buffer = std::malloc(data.count * type_size);
			std::memcpy(&((char*) send_buffer)[0], data.buffer, data.size);
			
			
			int* size_buffer;
			//int recv_size_count = 0;
			if(getRank() == root) {
				//recv_size_count = getSize();
				size_buffer = (int*) std::malloc(getSize() * sizeof(int));
			} else {
				size_buffer = (int*) std::malloc(0);
			}

			MPI_Gather(&data.count, 1, MPI_INT, size_buffer, 1, MPI_INT, root, comm);

			int* recvcounts;
			int* rdispls;
			int current_rdispls = 0;
			if(getRank() == root) {
				recvcounts = (int*) std::malloc(getSize()*sizeof(int));
				rdispls = (int*) std::malloc(getSize()*sizeof(int));
				for (int i = 0; i < getSize(); i++) {
					recvcounts[i] = size_buffer[i];
					rdispls[i] = current_rdispls;
					current_rdispls += recvcounts[i];
				}
			} else {
				recvcounts = (int*) std::malloc(0);
				rdispls = (int*) std::malloc(0);
			}
			void* recv_buffer = std::malloc(current_rdispls * type_size);

			MPI_Gatherv(send_buffer, data.count, type, recv_buffer, recvcounts, rdispls, type, root, comm);

			std::vector<DataPack> imported_data_pack;
			if(getRank() == root) {
				for (int i = 0; i < getSize(); i++) {
					imported_data_pack.emplace_back(recvcounts[i], type_size);
					DataPack& data_pack = imported_data_pack[i];

					std::memcpy(data_pack.buffer, &((char*) recv_buffer)[rdispls[i]], data_pack.size);
				}
			}
			std::free(send_buffer);
			std::free(size_buffer);
			std::free(recvcounts);
			std::free(rdispls);
			std::free(recv_buffer);
			return imported_data_pack;
	}

	/**
	 * fpmas::communication::MpiCommunicator destructor.
	 *
	 * Properly releases acquired MPI resources.
	 */
	MpiCommunicator::~MpiCommunicator() {
		MPI_Group_free(&this->group);
		if(this->comm_references == 1)
			MPI_Comm_free(&this->comm);
	}
}