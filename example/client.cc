#include <iostream>
using std::cout;
using std::endl;

#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "NetworkIO.hh"

int main(){
	auto network = NetworkIO::start();
	auto connection = network->connect("localhost",5555);

	RawTextMessage msg("Why hello there");
	connection->write(msg);

	std::this_thread::sleep_for(std::chrono::seconds(1));
}
