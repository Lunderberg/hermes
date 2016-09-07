#include "NetworkSocket.hh"

#include <iostream>

#include "NetworkIO.hh"

using asio::ip::tcp;

hermes::NetworkSocket::NetworkSocket(NetworkIO io,
                                     asio::ip::tcp::resolver::iterator endpoint)
  : m_io(io), m_socket(m_io.internals->io_service),
    m_read_loop_started(false), m_writer_running(false) {

  asio::async_connect(m_socket, endpoint,
  [this](asio::error_code ec, tcp::resolver::iterator) {
    if (!ec) {
      start_read_loop();
    }
  });
}

hermes::NetworkSocket::NetworkSocket(NetworkIO io,
                                     asio::ip::tcp::socket socket)
  : m_io(io), m_socket(std::move(socket)),
    m_writer_running(false) {

  m_io.internals->io_service.post( [this]() { start_read_loop(); });
}

hermes::NetworkSocket::~NetworkSocket() {
  std::unique_lock<std::mutex> lock(m_unacknowledged_mutex);
  m_all_messages_acknowledged.wait_for(
    lock, std::chrono::seconds(5),
    [this](){ return !m_socket.is_open() || m_unacknowledged_messages == 0; });

  m_socket.close();
  asio::error_code ec;
  m_socket.shutdown(asio::ip::tcp::socket::shutdown_both,ec);
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
  asio::async_read(m_socket,
                   asio::buffer(m_current_read.header.arr, header_size),
                   [this](asio::error_code ec, std::size_t /*length*/) {
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

                     } else {
                       m_socket.close();
                       m_received_message.notify_all();
                     }
                   });
}

void hermes::NetworkSocket::do_read_body() {
  m_current_read.body = std::string(); // In case it is a moved-from value
  m_current_read.body.resize(m_current_read.header.packed.size, '\0');

  asio::async_read(m_socket,
                   asio::buffer(&m_current_read.body[0], m_current_read.body.size()),
                   [this](asio::error_code ec, std::size_t /*length*/) {
                     if (!ec) {
                       write_acknowledge(m_current_read.header);
                       unpack_message();
                       do_read_header();
                     } else {
                       m_socket.close();
                       m_received_message.notify_all();
                     }
                   });
}

void hermes::NetworkSocket::unpack_message() {
  auto& unpacker = m_io.internals->message_templates.get_by_id(m_current_read.header.packed.id);
  auto unpacked = unpacker.unpack(m_current_read.body);
  m_current_read.body = std::string();

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
  m_io.internals->io_service.post(
    [this]() {
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
  asio::async_write(m_socket,
                    asio::buffer(m_current_write.header.arr, header_size),
                    [this](asio::error_code ec, std::size_t /*length*/) {
                      if (!ec) {
                        if(m_current_write.header.packed.acknowledge == 0) {
                          do_write_body();
                        } else {
                          do_write_header();
                        }
                      } else {
                        m_socket.close();
                      }
                    });
}

void hermes::NetworkSocket::do_write_body() {
  asio::async_write(m_socket,
                    asio::buffer(m_current_write.body.c_str(), m_current_write.body.size()),
                    [this](asio::error_code ec, std::size_t /*length*/) {
                      if(!ec) {
                        do_write_header();
                      } else {
                        m_socket.close();
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
