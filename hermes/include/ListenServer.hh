#ifndef _LISTENSERVER_H_
#define _LISTENSERVER_H_

#include <memory>
#include <mutex>
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
  /// Opens a port to listen on
  /**
     Shouldn't be called directly.
     Instead, use NetworkIO::listen.
   */
  ListenServer(NetworkIO io,
               asio::ip::tcp::endpoint endpoint);
  ~ListenServer();

  /// Returns true if there is a new connection waiting.
  bool HasNewConnection() {return m_connections.size();}

  /// Returns a new connection
  /**
     If no connection has been made, returns nullptr.
   */
  std::unique_ptr<NetworkSocket> GetConnection();

private:
  void do_accept();

  NetworkIO m_io;
  asio::ip::tcp::acceptor m_acceptor;
  asio::ip::tcp::socket m_socket;
  std::shared_ptr<MessageTemplates> m_message_templates;

  std::mutex m_mutex;
  std::deque<std::unique_ptr<NetworkSocket> > m_connections;
};
}

#endif /* _LISTENSERVER_H_ */
