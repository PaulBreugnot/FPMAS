#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define FPMAS_MIN_PROCS(test, comm, min) \
	if(comm.getSize() <= min) \
		std::cout << "\e[33m[ WARNING  ]\e[0m " test " test ignored : please use at least " #min " procs." << std::endl; \
	else

#endif
