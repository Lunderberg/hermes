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
    /// Constructs a socket
    /**
       Shouldn't need to be called directly.
       Instead, use NetworkIO::connect and ListenServer::GetConnection
     */
    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::resolver::iterator endpoint);

    /// Constructs a socket
    /**
       Shouldn't need to be called directly.
       Instead, use NetworkIO::connect and ListenServer::GetConnection
     */
    NetworkSocket(NetworkIO io,
                  asio::ip::tcp::socket socket);

    virtual ~NetworkSocket();

    /// Returns a message received from the socket
    /**
       If no message has been received, returns nullptr.
     */
    std::unique_ptr<UnpackedMessage> GetMessage();

    /// Returns a message received from the socket, waiting indefinitely
    /**
       Waits indefinitely for a message to be received.
       If the socket closes, will return a nullptr.
       Otherwise, will return a valid object.
     */
    std::unique_ptr<UnpackedMessage> WaitForMessage();

    /// Returns a message received from the socket, waiting the specified time.
    /**
       Waits up to the time specified for a message to be received.
       If the socket closes, or if timeout occurs, will return a nullptr.
       Otherwise, will return a valid object.
     */
    std::unique_ptr<UnpackedMessage> WaitForMessage(std::chrono::duration<double> duration);

    /// Write a message to the socket
    /**
       Returns immediately, asynchronously sending the message.
       The type being passed must have been previously been defined
         with NetworkIO::message_type.
     */
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

    /// Returns whether a message has been received
    bool HasNewMessage();

    /// Returns whether the socket is currently open
    /**
       The socket closes when it an error is given on async_read/write.
     */
    bool IsOpen();

    /// Returns whether a message is currently being sent
    /**
       Note that this only indicates whether all messages have been passed to the OS.
       The OS may not have sent the messages across the network,
         and the other computer may not have received the message.
     */
    bool SendInProgress();

    /// How many messages are queued to be written.
    int WriteMessagesQueued();

  private:
    /// Initializes socket settings, then starts the chain of async_read
    /**
       Sets the "linger" option, so the socket won't prematurely close.
       Calls do_read_header on the networking thread.
     */
    void start_read_loop();

    /// Write a packed message on the socket
    /**
       Called by write(), after packing the message.
     */
    void write_direct(Message message);

    /// Read a single header from the socket
    /**
       Reads into m_current_read.header.
       If the header is an acknowledge, record it and read another header.
       If the header is not an acknowledge, read the body of the message.
     */
    void do_read_header();

    /// Read the body of a message from the socket.
    /**
       Reads into m_current_read.body.
       On success, calls unpack_message(), then chains into do_read_header().
     */
    void do_read_body();

    /// Pops from m_read_messages, if something is available
    /**
       Assumes that the caller has already acquired the m_read_lock mutex.
       If no message is present, returns nullptr.
     */
    std::unique_ptr<UnpackedMessage> pop_if_available();

    /// Unpacks a message, places in m_read_messages
    /**
       Uses the unpacker stored in m_io.internals->message_templates.
     */
    void unpack_message();

    /// Writes an acknowledge of a full message received.
    /**
       Accepts the header of the message being acknowledged.
       The acknowledge is the same header, but with the acknowledge field set.
     */
    void write_acknowledge(network_header header);

    /// Start the writer, if the writer is not already running.
    void start_writer();

    /// Write a single header
    /**
       Pulls a message out of m_write_message,
         then writes its header onto the network.
       If the header is an acknowledge, chain into do_write_header.
       If the header is not an acknowledge, chain into do_write_body.
     */
    void do_write_header();

    /// Write the body of a message
    /**
       Writes the body from m_current_write.body.
       Should only be called from in do_write_header.
       On completion, chains into do_write_header.
     */
    void do_write_body();

    /// The NetworkIO running the socket.
    /**
       We use the unpackers defined here.
       Holding the NetworkIO keeps the networking thread open.
     */
    NetworkIO m_io;
    asio::ip::tcp::socket m_socket;

    /// Mutex for configuring the socket
    /**
       Can't start writing until the socket has been configured.
       This is used to delay writes until the network thread has configured the socket.
     */
    std::mutex m_open_mutex;
    std::condition_variable m_can_write;
    std::atomic_bool m_read_loop_started;

    /// The current message being read from the socket
    Message m_current_read;
    /// Additional messages, already having been read from the socket
    std::deque<std::unique_ptr<UnpackedMessage> > m_read_messages;
    /// A lock around m_read_messages
    std::mutex m_read_lock;
    /// Condition variable for waiting on m_read_messages to have something
    std::condition_variable m_received_message;

    /// Messages being queued up to write
    std::deque<Message> m_write_messages;
    /// The current message being written
    Message m_current_write;
    /// Whether or not the writer is currently running
    std::atomic_bool m_writer_running;
    /// Lock around m_write_messages
    std::mutex m_write_lock;

    /// Count of messages sent, but not acknowledged
    /**
       Not all operating systems allow checking whether there are TCP packets waiting to be sent.
       Therefore, we must rely on acknowledgement sent within the protocol itself.
       This is incremented on sending a message,
         and decrement on receiving an acknowledge.
     */
    std::atomic_int m_unacknowledged_messages;
    /// Unacknowledged mutex
    std::mutex m_unacknowledged_mutex;
    /// Called whenever the unacknowledged messages goes down to 0
    std::condition_variable m_all_messages_acknowledged;
  };
}



#endif /* _NETWORKSOCKET_H_ */
