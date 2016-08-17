#ifndef _LISTENSERVER_H_
#define _LISTENSERVER_H_

#include <memory>
#include <deque>

#include "asio.hpp"

#include "NetworkIO.hh"
#include "MessageTemplates.hh"

class MessageTemplates;

namespace hermes {
class NetworkIO;
class NetworkSocket;

class ListenServer {
public:
  ListenServer(std::shared_ptr<NetworkIO> io,
               asio::ip::tcp::endpoint endpoint,
               std::shared_ptr<MessageTemplates> templates);

  bool HasNewConnection() {return m_connections.size();}

  std::unique_ptr<NetworkSocket> GetConnection() {
    if(HasNewConnection()) {
      auto output = std::move(m_connections.front());
      m_connections.pop_front();
      return output;
    } else {
      return nullptr;
    }
  }

private:
  void do_accept();

  std::shared_ptr<NetworkIO> m_io;
  asio::ip::tcp::acceptor m_acceptor;
  asio::ip::tcp::socket m_socket;
  std::shared_ptr<MessageTemplates> m_message_templates;
  std::deque<std::unique_ptr<NetworkSocket> > m_connections;
};
}

#endif /* _LISTENSERVER_H_ */
