#include "ListenServer.hh"

#include <iostream>

#include "NetworkSocket.hh"
#include "MakeUnique.hh"

hermes::ListenServer::ListenServer(hermes::NetworkIO io,
                                   asio::ip::tcp::endpoint endpoint)
  : m_io(io), m_acceptor(io.internals->io_service,endpoint), m_socket(io.internals->io_service) {

  do_accept();
}

hermes::ListenServer::~ListenServer() {
  m_acceptor.cancel();
  m_acceptor.close();
}

void hermes::ListenServer::do_accept() {
  m_acceptor.async_accept(m_socket,
                          [this](asio::error_code ec) {
                            static int i=0;
                            i++;
                            if (!ec) {
                              auto connection = make_unique<hermes::NetworkSocket>(m_io,
                                                                                   std::move(m_socket));
                              std::lock_guard<std::mutex> lock(m_mutex);
                              m_connections.push_back(std::move(connection));
                            } else if (ec != asio::error::operation_aborted){
                              std::cout << "do_accept, #" << i << " ec: " << ec << "\t" << ec.message() << std::endl;
                            }

                            if (ec != asio::error::operation_aborted){
                              do_accept();
                            }
                          });
}

std::unique_ptr<hermes::NetworkSocket> hermes::ListenServer::GetConnection() {
  std::lock_guard<std::mutex> lock(m_mutex);

  if(HasNewConnection()) {
    auto output = std::move(m_connections.front());
    m_connections.pop_front();
    return output;
  } else {
    return nullptr;
  }
}
