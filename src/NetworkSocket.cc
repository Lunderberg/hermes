#include "NetworkSocket.hh"

using boost::asio::ip::tcp;

NetworkSocket::NetworkSocket(std::shared_ptr<boost::asio::io_service> io_service)
	: m_io_service(io_service), m_socket(*m_io_service){

	m_read_body.reserve(max_message_size);
}

NetworkSocket::~NetworkSocket(){
	m_socket.cancel();
	m_socket.close();
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
															auto new_message = Unpack(Message::message_id(m_read_header.id),
																												{m_read_body.begin(), m_read_body.end()} );
															m_read_messages.push_back(std::move(new_message));
															do_read_header();
														} else {
															m_socket.close();
														}
													});
}

void NetworkSocket::write(const Message& message){
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
	m_io_service->post(
										 [this,buffer](){
											 bool write_in_progress = !m_write_messages.empty();
											 m_write_messages.push_back(buffer);
											 if(!write_in_progress){
												 do_write();
											 };
										 });
}

void NetworkSocket::do_write(){
	// Write the buffer to the socket.
	boost::asio::async_write(m_socket,
													 boost::asio::buffer(m_write_messages.front().data(),
																							 m_write_messages.front().size()),
													 [this](boost::system::error_code ec, std::size_t length){
														 if(!ec){
															 m_write_messages.pop_front();
															 if(!m_write_messages.empty()){
																 do_write();
															 }
														 } else {
															 m_socket.close();
														 }
													 });
}
