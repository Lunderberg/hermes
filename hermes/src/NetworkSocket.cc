#include "NetworkSocket.hh"

#include <iostream>

#include "NetworkIO.hh"

using asio::ip::tcp;

hermes::NetworkSocket::NetworkSocket(NetworkIO io,
                                     asio::ip::tcp::resolver::iterator endpoint)
  : m_io(io), m_socket(m_io.internals->io_service),
    m_read_loop_started(false), m_current_message(nullptr),
    m_writer_running(false) {

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
  while (m_socket.is_open() &&
         m_unacknowledged_messages!=0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

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
                   asio::buffer(m_read_header.arr,header_size),
                   [this](asio::error_code ec, std::size_t /*length*/) {
                     if (!ec) {
                       if (m_read_header.packed.acknowledge==0) {
                         do_read_body();
                       } else {
                         m_unacknowledged_messages--;
                         do_read_header();
                       }

                     } else {
                       m_socket.close();
                     }
                   });
}

void hermes::NetworkSocket::do_read_body() {
  m_current_message = m_io.internals->message_templates.create_by_id(m_read_header.packed.id);

  asio::async_read(m_socket,
                          //asio::buffer(m_current_message->raw(),m_read_header.size()),
                   asio::buffer(m_current_message->raw(),m_current_message->size()),
                   [this](asio::error_code ec, std::size_t /*length*/) {
                     if (!ec) {
                       {
                         std::lock_guard<std::mutex> lock(m_read_lock);
                         m_read_messages.push_back(std::move(m_current_message));
                       }
                       write_acknowledge(m_read_header);
                       do_read_header();
                     } else {
                       m_socket.close();
                     }
                   });
}

void hermes::NetworkSocket::write_direct(const Message& message) {
  {
    std::unique_lock<std::mutex> lock(m_open_mutex);
    m_can_write.wait_for(lock, std::chrono::seconds(1),
                         [this]() { return bool(m_read_loop_started); });
  }

  // Make header
  network_header header;
  header.packed.size = message.size();
  header.packed.id = message.id();
  header.packed.acknowledge = 0;
  if (message.size() > max_message_size) {
    throw std::runtime_error("Message size exceeds maximum");
  }

  // Form full message from header and payload.
  std::vector<char> buffer;
  std::copy(header.arr, header.arr+header_size, std::back_inserter(buffer));
  std::copy(message.raw(), message.raw() + message.size(), std::back_inserter(buffer));

  {
    std::lock_guard<std::mutex> lock(m_write_lock);
    m_write_messages.push_back(buffer);
  }

  // Start the writing
  m_unacknowledged_messages++;
  m_io.internals->io_service.post([this]() {
      if (!m_writer_running) {
        m_writer_running = true;
        do_write();
      }
    });
}

void hermes::NetworkSocket::write_acknowledge(network_header header) {
  header.packed.acknowledge = 1;
  std::vector<char> buffer;
  std::copy(header.arr, header.arr+header_size, std::back_inserter(buffer));

  m_io.internals->io_service.post(
    [this,buffer]() {
      {
        std::lock_guard<std::mutex> lock(m_write_lock);
        m_write_messages.push_back(buffer);
      }
      if (!m_writer_running) {
        m_writer_running = true;
        do_write();
      }
    });
}

void hermes::NetworkSocket::do_write() {
  {
    std::lock_guard<std::mutex> lock(m_write_lock);
    m_current_write = m_write_messages.front();
  }

  // Write the buffer to the socket.
  asio::async_write(m_socket,
                    asio::buffer(m_current_write),
                    [this](asio::error_code ec, std::size_t /*length*/) {
                      if (!ec) {
                        bool continue_writing;
                        {
                          std::lock_guard<std::mutex> lock(m_write_lock);
                          m_write_messages.pop_front();
                          continue_writing = !m_write_messages.empty();
                        }
                        if (continue_writing) {
                          do_write();
                        } else {
                          m_writer_running = false;
                        }
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

std::unique_ptr<hermes::Message> hermes::NetworkSocket::GetMessage() {
  std::lock_guard<std::mutex> lock(m_read_lock);

  auto output = std::move(m_read_messages.front());
  m_read_messages.pop_front();
  return output;
}
