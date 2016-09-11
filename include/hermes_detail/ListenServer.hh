#ifndef _LISTENSERVER_H_
#define _LISTENSERVER_H_

#include <chrono>
#include <condition_variable>
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

  /// Get a new connection, return immediately.
  /**
     If no connection has been made, returns nullptr.
   */
  std::unique_ptr<NetworkSocket> GetConnection();

  /// Returns a new connection, waiting indefinitely for one.
  std::unique_ptr<NetworkSocket> WaitForConnection();

  /// Returns a new connection, waiting for the time specified.
  std::unique_ptr<NetworkSocket> WaitForConnection(std::chrono::duration<double> duration);

private:
  /// Pops a network socket off of m_connections
  /**
     Returns the first element from m_connections.
     Assumes that m_mutex has already been acquired by the caller.
   */
  std::unique_ptr<NetworkSocket> pop_if_available();

  void do_accept();

  NetworkIO m_io;
  asio::ip::tcp::acceptor m_acceptor;
  asio::ip::tcp::socket m_socket;
  std::shared_ptr<MessageTemplates> m_message_templates;

  std::mutex m_mutex;
  std::condition_variable m_has_new_connection;
  std::deque<std::unique_ptr<NetworkSocket> > m_connections;
};
}

#endif /* _LISTENSERVER_H_ */
