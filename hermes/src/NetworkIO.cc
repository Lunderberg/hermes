#include "NetworkIO.hh"

#include <stdexcept> // for std::runtime_error
#include <algorithm> // for std::copy
#include <iterator> // for std::back_insertor

#include "MakeUnique.hh"

using asio::ip::tcp;

std::shared_ptr<hermes::NetworkIO> hermes::NetworkIO::start() {
  //Can't use std::make_shared because the constructor is private
  return std::shared_ptr<hermes::NetworkIO>(new hermes::NetworkIO);
}

hermes::NetworkIO::NetworkIO()
  : m_thread(), m_io_service(new asio::io_service), m_work(*m_io_service),
    m_message_templates(std::make_shared<MessageTemplates>()) {

  m_thread = std::thread([this]() {
      while (true) {
        try {
          m_io_service->run();
        } catch (std::exception& e) {
          continue;
        }
        break;
      }
    });
    }

hermes::NetworkIO::~NetworkIO() {
  m_io_service->stop();
  m_thread.join();
}

std::unique_ptr<hermes::NetworkSocket> hermes::NetworkIO::connect(std::string server, int port) {
  return connect(server, std::to_string(port));
}

std::unique_ptr<hermes::NetworkSocket> hermes::NetworkIO::connect(std::string server, std::string port) {
  tcp::resolver resolver(*m_io_service);
  auto endpoint = resolver.resolve({server,port});
  return make_unique<hermes::NetworkSocket>(shared_from_this(), endpoint, m_message_templates);
}

std::unique_ptr<hermes::ListenServer> hermes::NetworkIO::listen(int port) {
  tcp::endpoint endpoint(tcp::v4(), port);
  return make_unique<hermes::ListenServer>(shared_from_this(), endpoint, m_message_templates);
}
