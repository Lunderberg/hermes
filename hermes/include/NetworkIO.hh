#ifndef _NETWORKIO_H_
#define _NETWORKIO_H_

#include <thread>
#include <string>
#include <memory>
#include <deque>

#include "asio.hpp"

#include "MessageTemplates.hh"

namespace hermes {
  class NetworkSocket;
  class ListenServer;

  /// Master class, from which sockets are opened.
  class NetworkIO {
  public:
    NetworkIO();
    ~NetworkIO();

    /// Connect to the port specified
    /**
       Returns a socket object, which can read or write messages
     */
    std::unique_ptr<NetworkSocket> connect(std::string server, int port);

    /// Connect to the port or service specified
    /**
       Returns a socket object, which can read or write messages
     */
    std::unique_ptr<NetworkSocket> connect(std::string server, std::string port);

    /// Listen on the specified port
    /**
       Opens the port, listens indefinitely.
     */
    std::unique_ptr<ListenServer> listen(int port);

    /// Defines a message that can be passed through any sockets opened from here.
    /**
       Template on the message type that will be passed.
       The id is the unique id of the type, in the message header.
     */
    template<typename T>
    void message_type(id_type id) {
      internals->message_templates.define<T>(id);
    }

  private:

    /// Struct containing all internal variables of the network_io
    /**
       Somewhat silly, but lets me declare NetworkIO on the stack,
         instead of needing to maintain it as a shared_ptr.
     */
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
