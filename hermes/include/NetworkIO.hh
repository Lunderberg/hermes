#ifndef _NETWORKIO_H_
#define _NETWORKIO_H_

#include <thread>
#include <string>
#include <memory>
#include <deque>

#include <iostream>

#include "asio.hpp"

#include "Message.hh"
#include "ListenServer.hh"
#include "NetworkSocket.hh"
#include "MessageTemplates.hh"

class NetworkSocket;
class ListenServer;

namespace hermes {
class NetworkIO : public std::enable_shared_from_this<NetworkIO> {
public:
  static std::shared_ptr<NetworkIO> start();
  std::shared_ptr<NetworkSocket> connect(std::string server, int port);
  std::shared_ptr<NetworkSocket> connect(std::string server, std::string port);
  std::shared_ptr<ListenServer> listen(int port);

  std::shared_ptr<asio::io_service> GetService() {return m_io_service;}

  ~NetworkIO();

  template<typename T>
  void message_type(id_type id) {
    m_message_templates->define<T>(id);
  }

private:
  NetworkIO();

  std::thread m_thread;
  std::shared_ptr<asio::io_service> m_io_service;
  asio::io_service::work m_work;

  std::shared_ptr<MessageTemplates> m_message_templates;
};
}

#endif /* _NETWORKIO_H_ */
