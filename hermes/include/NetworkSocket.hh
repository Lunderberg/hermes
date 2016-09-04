#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "asio.hpp"

#include "Message.hh"
#include "MessageTemplates.hh"
#include "NetworkIO.hh"

namespace hermes {
  class NetworkSocket {
  public:
    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::resolver::iterator endpoint);

    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::socket socket);

    virtual ~NetworkSocket();

    std::unique_ptr<Message> GetMessage();

    template<typename T>
    void write(const T& message) {
      auto msg_obj = m_io.internals->message_templates.create_by_class<T>();
      msg_obj->unpacked() = message;
      write_direct(*msg_obj);
    }

    bool HasNewMessage();
    bool IsOpen();
    bool SendInProgress();
    int WriteMessagesQueued();

  protected:
    void start_read_loop();
    void write_direct(const Message& message);

    void do_read_header();
    void do_read_body();
    void do_write();
    void write_acknowledge(network_header header);

    NetworkIO m_io;
    asio::ip::tcp::socket m_socket;

    std::mutex m_open_mutex;
    std::condition_variable m_can_write;
    std::atomic_bool m_read_loop_started;

    std::unique_ptr<Message> m_current_message;

    network_header m_read_header;
    std::deque<std::unique_ptr<Message> > m_read_messages;
    std::mutex m_read_lock;

    std::deque<std::vector<char> > m_write_messages;
    std::vector<char> m_current_write;
    std::atomic_bool m_writer_running;
    std::mutex m_write_lock;

    std::atomic_int m_unacknowledged_messages;
  };
}



#endif /* _NETWORKSOCKET_H_ */
