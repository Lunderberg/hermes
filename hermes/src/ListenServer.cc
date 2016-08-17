#include "ListenServer.hh"

#include "NetworkSocket.hh"
#include "MakeUnique.hh"

hermes::ListenServer::ListenServer(std::shared_ptr<hermes::NetworkIO> io,
                                   boost::asio::ip::tcp::endpoint endpoint,
                                   std::shared_ptr<MessageTemplates> templates)
  : m_io(io), m_acceptor(*io->GetService(),endpoint), m_socket(*io->GetService()),
    m_message_templates(templates) {
  do_accept();
}

void hermes::ListenServer::do_accept() {
  m_acceptor.async_accept(m_socket,
  [this](boost::system::error_code ec) {
    if (!ec) {
      auto connection = make_unique<hermes::NetworkSocket>(m_io,
                                                           std::move(m_socket),
                                                           m_message_templates);
      m_connections.push_back(std::move(connection));
    }
    do_accept();
  });
}
