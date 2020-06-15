#include "synchro/hard/termination.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::_;

using FPMAS::synchro::hard::Color;
using FPMAS::synchro::hard::Tag;
using FPMAS::synchro::hard::TerminationAlgorithm;

class TerminationTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<0, 4> comm;
		MockMutexServer<int> mutexServer;
		TerminationAlgorithm<MockMpi> termination {comm};
};

TEST_F(TerminationTest, rank_0_white_tokens) {
	EXPECT_CALL(mutexServer, getEpoch).WillRepeatedly(Return(Epoch::EVEN));
	EXPECT_CALL(mutexServer, setEpoch(Epoch::ODD));

	EXPECT_CALL(
			const_cast<MockMpi<Color>&>(termination.getColorMpi()),
			send(Color::WHITE, 3, Tag::TOKEN))
		.Times(1);

	EXPECT_CALL(comm, Iprobe(1, Tag::TOKEN, _))
		.WillRepeatedly(Return(1));

	EXPECT_CALL(
		const_cast<MockMpi<Color>&>(termination.getColorMpi()), recv(_))
		.WillOnce(Return(Color::WHITE));

	// Sends end messages
	EXPECT_CALL(comm, send(1, Tag::END)).Times(1);
	EXPECT_CALL(comm, send(2, Tag::END)).Times(1);
	EXPECT_CALL(comm, send(3, Tag::END)).Times(1);

	termination.terminate(mutexServer);
}