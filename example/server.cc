#include <iostream>
using std::cout;
using std::endl;
#include <string>

#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "NetworkIO.hh"

int main(){
	NetworkIO n;
	auto listener = n.listen(5555);

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
