#include <iostream>
#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"
#include "NetworkIO.hh"

int main(){
  auto network = hermes::NetworkIO::start();
  network->message_type<IntegerMessage>(1);
  network->message_type<RawTextMessage>(2);

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
        switch(message->id()) {
          case 1: {
            IntegerMessage m = *message;
            std::cout << "Integer message: " << m.value << std::endl;
          }
            break;

          case 2: {
            RawTextMessage m = *message;
            m.buf[79] = '\0';
            std::cout << "Text: " << m.buf << std::endl;
          }
            break;

          default:
            std::cout << "Unknown message type: " << message->id() << std::endl;
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }
}
