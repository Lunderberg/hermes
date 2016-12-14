#define ASIO_STANDALONE

#include "hermes_detail/NetworkIO.hh"

#include "hermes_detail/ListenServer.hh"
#include "hermes_detail/MakeUnique.hh"
#include "hermes_detail/NetworkSocket.hh"

using asio::ip::tcp;

hermes::NetworkIO::NetworkIO()
  : internals(nullptr) {

  internals = std::make_shared<internals_t>();
  internals->thread = std::thread(
    [this]() {
      while (true) {
        try {
          internals->io_service.run();
        } catch (std::exception& e) {
          continue;
        }
        break;
      }
    });
}

hermes::NetworkIO::~NetworkIO() { }

std::unique_ptr<hermes::NetworkSocket> hermes::NetworkIO::connect(std::string server, int port) {
  return connect(server, std::to_string(port));
}

std::unique_ptr<hermes::NetworkSocket> hermes::NetworkIO::connect(std::string server, std::string port) {
  tcp::resolver resolver(internals->io_service);
  auto endpoint = resolver.resolve({server,port});
  return make_unique<hermes::NetworkSocket>(*this, endpoint);
}

std::unique_ptr<hermes::ListenServer> hermes::NetworkIO::listen(int port) {
  tcp::endpoint endpoint(tcp::v4(), port);
  return make_unique<hermes::ListenServer>(*this, endpoint);
}
