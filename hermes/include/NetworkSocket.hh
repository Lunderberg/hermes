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
#include "UnpackedMessage.hh"

namespace hermes {
  class NetworkSocket {
  public:
    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::resolver::iterator endpoint);

    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::socket socket);

    virtual ~NetworkSocket();

    std::unique_ptr<UnpackedMessage> GetMessage();

    template<typename T>
    void write(const T& obj) {
      auto& unpacker = m_io.internals->message_templates.get_by_class<T>();
      Message message;
      message.body = unpacker.pack(obj);

      message.header.packed.size = message.body.size();
      message.header.packed.id = unpacker.id();
      message.header.packed.acknowledge = 0;

      write_direct(std::move(message));
    }

    bool HasNewMessage();
    bool IsOpen();
    bool SendInProgress();
    int WriteMessagesQueued();

  private:
    void start_read_loop();
    void write_direct(Message message);

    void do_read_header();
    void do_read_body();
    void unpack_message();
    void write_acknowledge(network_header header);

    void start_writer();
    void do_write_header();
    void do_write_body();

    NetworkIO m_io;
    asio::ip::tcp::socket m_socket;

    std::mutex m_open_mutex;
    std::condition_variable m_can_write;
    std::atomic_bool m_read_loop_started;

    Message m_current_read;
    std::deque<std::unique_ptr<UnpackedMessage> > m_read_messages;
    std::mutex m_read_lock;

    std::deque<Message> m_write_messages;
    Message m_current_write;
    std::atomic_bool m_writer_running;
    std::mutex m_write_lock;

    std::atomic_int m_unacknowledged_messages;
  };
}



#endif /* _NETWORKSOCKET_H_ */
