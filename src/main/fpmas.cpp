#include <iostream> 

#include "zoltan_cpp.h"
#include "communication/communication.h"
#include <mpi.h>
#include "utils/config.h"

using FPMAS::communication::MpiCommunicator;

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	// Communication
	// Communication communication;
	MpiCommunicator mainCommunicator;

	//Initialize the Zoltan library with a C language call
	float version;
	Zoltan_Initialize(argc, argv, &version);

	//Dynamically create Zoltan object.
	MpiCommunicator zoltanCommunicator;
	Zoltan *zz = new Zoltan(zoltanCommunicator.getMpiComm());
	FPMAS::config::zoltan_config(zz);


	//Several lines of code would follow, working with zz
	if(mainCommunicator.getRank() == 0) {
		std::cout << "" << std::endl;
		std::cout << "███████╗    ██████╗ ███╗   ███╗ █████╗ ███████╗" << std::endl;
		std::cout << "██╔════╝    ██╔══██╗████╗ ████║██╔══██╗██╔════╝" << std::endl;
		std::cout << "█████╗█████╗██████╔╝██╔████╔██║███████║███████╗" << std::endl;
		std::cout << "██╔══╝╚════╝██╔═══╝ ██║╚██╔╝██║██╔══██║╚════██║" << std::endl;
		std::cout << "██║         ██║     ██║ ╚═╝ ██║██║  ██║███████║" << std::endl;
		std::cout << "╚═╝         ╚═╝     ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝" << std::endl;
		std::cout << "Version 2.0" << std::endl;
		std::cout << "" << std::endl;
		std::cout << "-------------------------------------------------" << std::endl;
		//    std::cout << "- NB TimeStep      : " << sparam->getMax_timestep() << std::endl;
		std::cout << "- NB Proc          : " << mainCommunicator.getSize() << std::endl;
		//  std::cout << "- NB Total Agent   : " << sparam->getTotal_agent()*communication->getSize() << std::endl;
		//std::cout << "- Optimisation     : " << sparam->getOptimisation() << std::endl;
		// if(sparam->getOptimisation() == 1) {
		//    std::cout << "- Ack period       : " << sparam->getPeriod_ack() << std::endl;
		//}
		//std::cout << "- Seed             : " << sparam->getSeed() << std::endl;
		std::cout << "-------------------------------------------------" << std::endl;
	}

	//Explicitly delete the Zoltan object
	delete zz;
	MPI_Finalize();
}
