#include <iostream>
using std::cout;
using std::endl;

#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"
#include "NetworkIO.hh"

int main(){
	auto network = NetworkIO::start();
	auto listener = network->listen(5555);

	while(true){
		// Wait for a connection to be made
		while(!listener->HasNewConnection()){
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		auto connection = listener->GetConnection();

		// Until the connection is closed, read messages
		while(connection->IsOpen() || connection->HasNewMessage()){
			if(connection->HasNewMessage()){
				// Handle each message according to its type
				auto message = connection->GetMessage();
				if(auto m = std::dynamic_pointer_cast<RawTextMessage>(message)){
					cout << "String message: " << m->GetText() << endl;
				} else if (auto m = std::dynamic_pointer_cast<IntegerMessage>(message)){
					cout << "Integer message: " << m->GetValue() << endl;
				} else {
					cout << "Unknown message: " << message->Pack() << endl;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}


	}
}
