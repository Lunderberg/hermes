#define ASIO_STANDALONE

#include "hermes_detail/NetworkSocket.hh"

#include <iostream>

#include "hermes_detail/NetworkIO.hh"

using asio::ip::tcp;

hermes::NetworkSocket::NetworkSocket(NetworkIO io,
                                     asio::ip::tcp::resolver::iterator endpoint)
  : m_io(io), m_socket(m_io.internals->io_service),
    m_read_loop_started(false), m_callbacks_running(0), m_writer_running(false),
    m_unacknowledged_messages(0) {

  CallbackCounter counter(this);
  asio::async_connect(m_socket, endpoint,
                      [this,counter](asio::error_code ec, tcp::resolver::iterator) {
                        if (!ec) {
                          start_read_loop();
                        }
                      });
}

hermes::NetworkSocket::NetworkSocket(NetworkIO io,
                                     asio::ip::tcp::socket socket)
  : m_io(io), m_socket(std::move(socket)),
    m_read_loop_started(false), m_callbacks_running(0), m_writer_running(false),
    m_unacknowledged_messages(0) {

  CallbackCounter counter(this);
  m_io.internals->io_service.post( [this,counter]() { start_read_loop(); });
}

hermes::NetworkSocket::~NetworkSocket() {
  std::unique_lock<std::mutex> lock(m_unacknowledged_mutex);
  m_all_messages_acknowledged.wait_for(
    lock, std::chrono::seconds(5),
    [this](){ return !m_socket.is_open() || m_unacknowledged_messages == 0; });

  close_socket();

  std::unique_lock<std::mutex> lock_all_finished(m_all_callbacks_finished_mutex);
  m_all_callbacks_finished.wait(lock_all_finished,
                                [this] () { return m_callbacks_running==0; } );
}

void hermes::NetworkSocket::close_socket() {
  std::unique_lock<std::mutex> lock(m_close_mutex);

  if(m_socket.is_open()) {
    asio::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both,ec);

    try {
      m_socket.close();
    } catch (std::system_error&) { }
  }

  m_socket_closed.notify_all();
  m_received_message.notify_all();
}

void hermes::NetworkSocket::WaitForClose() {
  std::unique_lock<std::mutex> lock(m_close_mutex);
  m_socket_closed.wait(lock, [this](){ return !m_socket.is_open(); });
}

void hermes::NetworkSocket::start_read_loop() {
  std::unique_lock<std::mutex> lock(m_open_mutex);
  m_read_loop_started = true;
  m_can_write.notify_all();

  asio::socket_base::linger option(true,1000);
  m_socket.set_option(option);
  do_read_header();
}

void hermes::NetworkSocket::do_read_header() {
  CallbackCounter counter(this);
  asio::async_read(m_socket,
                   asio::buffer(m_current_read.header.arr, header_size),
                   [this,counter](asio::error_code ec, std::size_t /*length*/) {
                     if (!ec) {
                       if (m_current_read.header.packed.acknowledge==0) {
                         do_read_body();
                       } else {
                         m_unacknowledged_messages--;
                         if(m_unacknowledged_messages == 0) {
                           m_all_messages_acknowledged.notify_one();
                         }
                         do_read_header();
                       }

                     } else if (ec != asio::error::operation_aborted){
                       close_socket();
                     }
                   });
}

void hermes::NetworkSocket::do_read_body() {
  m_current_read.body = std::string(); // In case it is a moved-from value
  m_current_read.body.resize(m_current_read.header.packed.size, '\0');

  CallbackCounter counter(this);
  asio::async_read(m_socket,
                   asio::buffer(&m_current_read.body[0], m_current_read.body.size()),
                   [this,counter](asio::error_code ec, std::size_t /*length*/) {
                     if (!ec) {
                       write_acknowledge(m_current_read.header);
                       unpack_message();
                       do_read_header();
                     } else if (ec != asio::error::operation_aborted){
                       close_socket();
                     }
                   });
}

void hermes::NetworkSocket::unpack_message() {
  auto& unpacker = m_io.internals->message_templates.get_by_id(m_current_read.header.packed.id);
  auto unpacked = unpacker.unpack(m_current_read.body);
  m_current_read.body = std::string();

  std::lock_guard<std::mutex> lock_callbacks(m_callback_mutex);
  for(auto& callback : m_callbacks) {
    bool res = callback->apply_on(*unpacked);
    if(res) {
      return;
    }
  }

  std::lock_guard<std::mutex> lock(m_read_lock);
  m_read_messages.push_back(std::move(unpacked));
  m_received_message.notify_one();
}

void hermes::NetworkSocket::write_direct(Message message) {
  {
    std::unique_lock<std::mutex> lock(m_open_mutex);
    m_can_write.wait_for(lock, std::chrono::seconds(1),
                         [this]() { return bool(m_read_loop_started); });
  }

  if (message.header.packed.size != message.body.size()) {
    throw std::runtime_error("Incorrect message header");
  }
  if (message.header.packed.size > max_message_size) {
    throw std::runtime_error("Message size exceeds maximum");
  }

  {
    std::lock_guard<std::mutex> lock(m_write_lock);
    m_write_messages.push_back(std::move(message));
  }

  // Start the writing
  m_unacknowledged_messages++;
  start_writer();
}

void hermes::NetworkSocket::start_writer() {
  CallbackCounter counter(this);
  m_io.internals->io_service.post(
    [this,counter]() {
      if (!m_writer_running) {
        m_writer_running = true;
        do_write_header();
      }
    });
}

void hermes::NetworkSocket::write_acknowledge(network_header header) {
  header.packed.acknowledge = 1;
  Message message;
  message.header = header;

  {
    std::lock_guard<std::mutex> lock(m_write_lock);
    m_write_messages.push_back(message);
  }

  start_writer();
}

void hermes::NetworkSocket::do_write_header() {
  {
    std::lock_guard<std::mutex> lock(m_write_lock);
    if(m_write_messages.size()) {
      m_current_write = std::move(m_write_messages.front());
      m_write_messages.pop_front();
    } else {
      m_writer_running = false;
      return;
    }
  }

  // Write the buffer to the socket.
  CallbackCounter counter(this);
  asio::async_write(m_socket,
                    asio::buffer(m_current_write.header.arr, header_size),
                    [this,counter](asio::error_code ec, std::size_t /*length*/) {
                      if (!ec) {
                        if(m_current_write.header.packed.acknowledge == 0) {
                          do_write_body();
                        } else {
                          do_write_header();
                        }
                      } else if (ec != asio::error::operation_aborted){
                        close_socket();
                      }
                    });
}

void hermes::NetworkSocket::do_write_body() {
  CallbackCounter counter(this);
  asio::async_write(m_socket,
                    asio::buffer(m_current_write.body.c_str(), m_current_write.body.size()),
                    [this,counter](asio::error_code ec, std::size_t /*length*/) {
                      if(!ec) {
                        do_write_header();
                      } else if (ec != asio::error::operation_aborted){
                        close_socket();
                      }
                    });
}

bool hermes::NetworkSocket::HasNewMessage() {
  std::lock_guard<std::mutex> lock(m_read_lock);

  return m_read_messages.size();
}

int hermes::NetworkSocket::WriteMessagesQueued() {
  std::lock_guard<std::mutex> lock(m_write_lock);
  return m_write_messages.size();
}

bool hermes::NetworkSocket::IsOpen() {
  return m_socket.is_open();
}

std::unique_ptr<hermes::UnpackedMessage> hermes::NetworkSocket::GetMessage() {
  std::lock_guard<std::mutex> lock(m_read_lock);
  return pop_if_available();
}

std::unique_ptr<hermes::UnpackedMessage> hermes::NetworkSocket::WaitForMessage() {
  std::unique_lock<std::mutex> lock(m_read_lock);
  m_received_message.wait(lock, [this]() { return m_read_messages.size() || !IsOpen(); } );
  return pop_if_available();
}

std::unique_ptr<hermes::UnpackedMessage>
hermes::NetworkSocket::WaitForMessage(std::chrono::duration<double> duration) {
  std::unique_lock<std::mutex> lock(m_read_lock);
  m_received_message.wait_for(lock, duration,
                              [this]() { return m_read_messages.size() || !IsOpen(); } );
  return pop_if_available();
}

std::unique_ptr<hermes::UnpackedMessage> hermes::NetworkSocket::pop_if_available() {
  if(m_read_messages.size()) {
    auto output = std::move(m_read_messages.front());
    m_read_messages.pop_front();
    return output;
  } else {
    return nullptr;
  }
}

void hermes::NetworkSocket::initialize_callback() {
  std::unique_ptr<MessageCallback> new_callback = nullptr;
  {
    std::lock_guard<std::mutex> lock(m_new_callback_mutex);
    new_callback = std::move(m_new_callbacks.front());
    m_new_callbacks.pop_front();
  }

  std::lock_guard<std::mutex> lock_callbacks(m_callback_mutex);
  std::lock_guard<std::mutex> lock_messages(m_read_lock);

  // Try callback on all messages, remove any that return true.
  m_read_messages.erase(std::remove_if(m_read_messages.begin(), m_read_messages.end(),
                                       [&](std::unique_ptr<UnpackedMessage>& msg) {
                                         return new_callback->apply_on(*msg);
                                       }),
                        m_read_messages.end());

  m_callbacks.push_back(std::move(new_callback));
}
