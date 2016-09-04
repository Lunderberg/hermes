#ifndef _NETWORKIO_H_
#define _NETWORKIO_H_

#include <thread>
#include <string>
#include <memory>
#include <deque>

#include <iostream>

#include "asio.hpp"

#include "Message.hh"
#include "MessageTemplates.hh"

namespace hermes {
  class NetworkSocket;
  class ListenServer;

  class NetworkIO {
  public:
    NetworkIO();
    ~NetworkIO();

    std::unique_ptr<NetworkSocket> connect(std::string server, int port);
    std::unique_ptr<NetworkSocket> connect(std::string server, std::string port);
    std::unique_ptr<ListenServer> listen(int port);

    template<typename T>
    void message_type(id_type id) {
      internals->message_templates.define<T>(id);
    }

  private:

    struct internals_t {
      template<typename T>
      internals_t(T&& t)
        : work(io_service), thread(std::forward<T>(t)) { }

      ~internals_t() {
        io_service.stop();
        thread.join();
      }

      asio::io_service io_service;
      asio::io_service::work work;
      MessageTemplates message_templates;
      std::thread thread;
    };

    std::shared_ptr<internals_t> internals;
    friend class NetworkSocket;
    friend class ListenServer;
  };
}

#include "NetworkSocket.hh"
#include "ListenServer.hh"

#endif /* _NETWORKIO_H_ */
