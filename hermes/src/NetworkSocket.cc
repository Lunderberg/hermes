#include "NetworkSocket.hh"

#include "NetworkIO.hh"

using boost::asio::ip::tcp;

NetworkSocket::NetworkSocket(std::shared_ptr<NetworkIO> io,
														 boost::asio::ip::tcp::resolver::iterator endpoint) :
	m_io(io), m_socket(*m_io->GetService()), m_is_writing(false) {

	m_read_body.reserve(max_message_size);

	boost::asio::async_connect(m_socket, endpoint,
														 [this](boost::system::error_code ec, tcp::resolver::iterator){
															 if(!ec){
																 do_read_header();
															 }
														 });
}

NetworkSocket::NetworkSocket(std::shared_ptr<NetworkIO> io,
														 boost::asio::ip::tcp::socket socket) :
	m_io(io), m_socket(std::move(socket)), m_is_writing(false) {

	m_io->GetService()->post(
													 [this](){
														 do_read_header();
													 });
}

NetworkSocket::~NetworkSocket(){
	m_socket.close();
	boost::system::error_code ec;
	m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both,ec);
}

void NetworkSocket::do_read_header(){
	boost::asio::async_read(m_socket,
													boost::asio::buffer(m_read_header.arr,header_size),
													[this](boost::system::error_code ec, std::size_t length){
														if(!ec){
															do_read_body();
														} else {
															m_socket.close();
														}
													});
}

void NetworkSocket::do_read_body(){
	m_read_body.resize(m_read_header.size);
	boost::asio::async_read(m_socket,
													boost::asio::buffer(m_read_body.data(),m_read_header.size),
													[this](boost::system::error_code ec, std::size_t length){
														if(!ec){
															auto new_message = Message::Unpack(m_read_header.id,
																																 {m_read_body.begin(), m_read_body.end()} );
															m_read_messages.push_back(std::move(new_message));
															do_read_header();
														} else {
															m_socket.close();
														}
													});
}

void NetworkSocket::write(const Message& message){

	boost::asio::socket_base::linger option(true,1000);
	m_socket.set_option(option);

	// Make header
	network_header header;
	std::string packed = message.Pack();
	header.size = packed.size();
	header.id = message.GetID();
	if(header.size > max_message_size){
		throw std::runtime_error("Message size exceeds maximum");
	}

	// Form full message from header and payload.
	std::vector<char> buffer;
	std::copy(header.arr, header.arr+header_size, std::back_inserter(buffer));
	std::copy(packed.begin(), packed.end(), std::back_inserter(buffer));

	// Start the writing
	m_is_writing = true;
	m_io->GetService()->post(
													 [this,buffer](){
														 {
															 std::lock_guard<std::recursive_mutex> lock(m_write_lock);
															 m_write_messages.push_back(buffer);
														 }
														 if(!m_writer_running){
															 m_writer_running = true;
															 do_write();
														 };
													 });
}

void NetworkSocket::do_write(){
	{
		std::lock_guard<std::recursive_mutex> lock(m_write_lock);
		m_current_write = m_write_messages.front();
	}

	// Write the buffer to the socket.
	boost::asio::async_write(m_socket,
													 boost::asio::buffer(m_current_write),
													 [this](boost::system::error_code ec, std::size_t length){
														 if(!ec){
															 bool continue_writing;
															 {
																 std::lock_guard<std::recursive_mutex> lock(m_write_lock);
																 m_write_messages.pop_front();
																 continue_writing = !m_write_messages.empty();
															 }
															 if(continue_writing){
																 do_write();
															 } else {
																 m_is_writing = false;
																 m_writer_running = false;
															 }
														 } else {
															 m_socket.close();
														 }
													 });
}

bool NetworkSocket::HasNewMessage(){
	std::lock_guard<std::recursive_mutex> lock(m_read_lock);

	return m_read_messages.size();
}

bool NetworkSocket::SendInProgress(){
	return m_is_writing;
}

int NetworkSocket::WriteMessagesQueued(){
	return m_write_messages.size();
}

bool NetworkSocket::IsOpen(){
	return m_socket.is_open();
}

std::shared_ptr<Message> NetworkSocket::GetMessage(){
	std::lock_guard<std::recursive_mutex> lock(m_read_lock);

	auto output = m_read_messages.front();
	m_read_messages.pop_front();
	return output;
}
