#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <deque>
#include <vector>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <mutex>

#include "asio.hpp"

#include "Message.hh"
#include "MessageTemplates.hh"

namespace hermes {
  class NetworkIO;
  class MessageTemplates;

  class NetworkSocket {
  public:
    NetworkSocket(std::shared_ptr<NetworkIO> io,
                  asio::ip::tcp::resolver::iterator endpoint,
                  std::shared_ptr<MessageTemplates> templates);

    NetworkSocket(std::shared_ptr<NetworkIO> io,
                  asio::ip::tcp::socket socket,
                  std::shared_ptr<MessageTemplates> templates);

    virtual ~NetworkSocket();

    std::unique_ptr<Message> GetMessage();

    template<typename T>
    void write(const T& message) {
      auto msg_obj = m_message_templates->create_by_class<T>();
      msg_obj->unpacked() = message;
      write_direct(*msg_obj);
    }

    bool HasNewMessage();
    bool IsOpen();
    bool SendInProgress();
    int WriteMessagesQueued();

  protected:
    void write_direct(const Message& message);

    void do_read_header();
    void do_read_body();
    void do_write();
    void write_acknowledge(network_header header);

    std::shared_ptr<NetworkIO> m_io;
    asio::ip::tcp::socket m_socket;
    std::shared_ptr<MessageTemplates> m_message_templates;

    std::unique_ptr<Message> m_current_message;

    network_header m_read_header;
    std::deque<std::unique_ptr<Message> > m_read_messages;
    std::recursive_mutex m_read_lock;

    std::deque<std::vector<char> > m_write_messages;
    std::vector<char> m_current_write;
    std::atomic_bool m_writer_running;
    std::recursive_mutex m_write_lock;

    std::atomic_int m_unacknowledged_messages;
  };
}



#endif /* _NETWORKSOCKET_H_ */
