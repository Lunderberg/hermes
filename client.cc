#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "IntegerMessage.hh"
#include "Message.hh"
#include "NetworkIO.hh"
#include "RawTextMessage.hh"

int main(){
  // Start a connection
  auto network = hermes::NetworkIO::start();
  network->message_type<IntegerMessage>(1);
  network->message_type<RawTextMessage>(2);

  auto connection = network->connect("localhost",5555);


  {
    // Send an IntegerMessage
    IntegerMessage msg;
    msg.value = 42;
    connection->write(msg);

  }

  {
    // Send a RawTextMessage
    RawTextMessage msg;
    strncpy(msg.buf, "Why hello there", 80);
    connection->write(msg);
  }

}
