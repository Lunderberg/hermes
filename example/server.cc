#include <iostream>
using std::cout;
using std::endl;

#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "NetworkIO.hh"

int main(){
	auto network = NetworkIO::start();
	auto listener = network->listen(5555);

	while(true){
		while(!listener->HasNewConnection()){
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		auto connection = listener->GetConnection();
		while(!connection->HasNewMessage()){
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		auto message = connection->GetMessage();
		cout << message->Pack() << endl;
	}
}
