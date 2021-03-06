#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "asio.hpp"

#include "Message.hh"
#include "MessageCallback.hh"
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

    /// Waits until the socket has closed
    /**
       Useful if callbacks have been defined for all message types,
       and the socket should now simply react to incoming messages.
     */
    void WaitForClose();

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
      message.body = unpacker.pack(&obj);

      message.header.packed.size = message.body.size();
      message.header.packed.id = unpacker.id();
      message.header.packed.acknowledge = 0;

      write_direct(std::move(message));
    }

    /// Adds a callback for a given message type.
    template<typename T>
    void add_callback(std::function<void(T&)> func) {
      add_callback<T>([func](std::unique_ptr<T> obj) {
          func(*obj);
        });
    }

    /// Adds a callback for a given message type.
    template<typename T>
    void add_callback(std::function<void(std::unique_ptr<T>)> func) {
      auto callback = make_unique<MessageCallbackType<T> >(func);
      std::lock_guard<std::mutex> lock(m_new_callback_mutex);
      m_new_callbacks.push_back(std::move(callback));
      CallbackCounter counter(this);
      m_io.internals->io_service.post(
        [this,counter]() { initialize_callback(); }
      );
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
    /// Helper struct, keeping track of the number of callbacks registered
    /**
       We can't let the NetworkSocket destructor end until all callbacks refering to it are done.
       Each lambda function callback will hold one of these by value.
       When the labmda function is destructed, it will decrease the callback counter.
     */
    struct CallbackCounter {
      CallbackCounter(NetworkSocket* socket)
        : socket(socket) {
        socket->m_callbacks_running++;
      }

      CallbackCounter(const CallbackCounter& other)
        : CallbackCounter(other.socket) { }

      CallbackCounter& operator=(const CallbackCounter& other) = delete;

      ~CallbackCounter() {
        std::lock_guard<std::mutex> lock(socket->m_all_callbacks_finished_mutex);
        socket->m_callbacks_running--;
        if(!socket->m_callbacks_running) {
          socket->m_all_callbacks_finished.notify_all();
        }
      }

      NetworkSocket* socket;
    };


    /// Closes the socket.
    /**
       Happens either on destruction of the NetworkSocket object,
       or when an error occurs on read/write
     */
    void close_socket();

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

    /// Initialize a single callback
    /**
       Messages may have arrived between the opening of the socket and the defined of a callback.
       Therefore, a callback must be checked against all queued messages
         before entering general use.
     */
    void initialize_callback();

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

    /// Mutex for waiting on m_all_callbacks_finished
    std::mutex m_all_callbacks_finished_mutex;
    /// Triggered when all callbacks have finished
    std::condition_variable m_all_callbacks_finished;
    /// Number of callbacks currently submitted
    std::atomic_int m_callbacks_running;

    /// Mutex for waiting on socket close
    std::mutex m_close_mutex;
    /// Condition variable for waiting on the socket to close
    std::condition_variable m_socket_closed;

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

    /// List of callbacks defined but not yet initialized
    std::deque<std::unique_ptr<MessageCallback> > m_new_callbacks;
    /// Mutex around new callbacks
    std::mutex m_new_callback_mutex;
    /// List of initialized callbacks
    std::vector<std::unique_ptr<MessageCallback> > m_callbacks;
    /// Mutex around initialized callbacks
    std::mutex m_callback_mutex;
  };
}



#endif /* _NETWORKSOCKET_H_ */
