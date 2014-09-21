#include "ClientNetworkSocket.hh"

using boost::asio::ip::tcp;

ClientNetworkSocket::ClientNetworkSocket(std::shared_ptr<boost::asio::io_service> io_service,
																				 boost::asio::ip::tcp::resolver::iterator endpoint)
	: NetworkSocket(io_service) {

	boost::asio::async_connect(m_socket, endpoint,
														 [this](boost::system::error_code ec, tcp::resolver::iterator){
															 if(!ec){
																 do_read_header();
															 }
														 });
}
