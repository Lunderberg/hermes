#include <iostream>
using std::cout;
using std::endl;

#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"
#include "NetworkIO.hh"

int main(){
	// Start a connection
	auto network = NetworkIO::start();
	auto connection = network->connect("localhost",5555);

	// Send a RawTextMessage
	RawTextMessage msg("Why hello there");
	connection->write(msg);

	// Send an IntegerMessage
	IntegerMessage msg2(42);
	connection->write(msg2);
}
