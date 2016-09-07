#include <iostream>
#include <thread>
#include <chrono>

#include "RawTextMessage.hh"
#include "IntegerMessage.hh"
#include "NetworkIO.hh"

void handle_message(hermes::UnpackedMessage& message) {
  if(auto m = message.view<IntegerMessage>()) {
    std::cout << "Integer message: " << m->value << std::endl;
  } else if (auto m = message.view<RawTextMessage>()) {
    m->buf[79] = '\0';
    std::cout << "Text: " << m->buf << std::endl;
  } else {
    std::cout << "Unknown message type" << std::endl;
  }
}

int main(){
  hermes::NetworkIO network;
  network.message_type<IntegerMessage>(1);
  network.message_type<RawTextMessage>(2);

  auto listener = network.listen(5555);

  while(true){
    // Wait for a connection to be made
    auto connection = listener->WaitForConnection();

    // Until the connection is closed, read messages
    while(connection->IsOpen() || connection->HasNewMessage()){
      auto message = connection->WaitForMessage();

      if(message) {
        handle_message(*message);
      } else {
        break;
      }
    }
  }
}
