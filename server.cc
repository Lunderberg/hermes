#include <iostream>
#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"
#include "NetworkIO.hh"

int main(){
  hermes::NetworkIO network;
  network.message_type<IntegerMessage>(1);
  network.message_type<RawTextMessage>(2);

  auto listener = network.listen(5555);

  while(true){
    // Wait for a connection to be made
    auto connection = listener->WaitForConnection();


    connection->add_callback<IntegerMessage>([](IntegerMessage& msg) {
        std::cout << "Integer message: " << msg.value << std::endl;
      });

    connection->add_callback<RawTextMessage>([](RawTextMessage& msg) {
        msg.buf[79] = '\0';
        std::cout << "Text: " << msg.buf << std::endl;
      });

    // Wait until the connection is closed.
    while(connection->IsOpen() || connection->HasNewMessage()){
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}
