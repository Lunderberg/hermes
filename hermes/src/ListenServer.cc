#include "ListenServer.hh"

ListenServer::ListenServer(std::shared_ptr<boost::asio::io_service> io_service,
													 boost::asio::ip::tcp::endpoint endpoint) :
	m_io_service(io_service), m_acceptor(*io_service,endpoint), m_socket(*io_service){
	do_accept();
}

void ListenServer::do_accept(){
	m_acceptor.async_accept(m_socket,
													[this](boost::system::error_code ec){
														if(!ec){
															auto connection = std::make_shared<NetworkSocket>(m_io_service,
																																								std::move(m_socket));
															m_connections.push_back(connection);
														}
														do_accept();
													});
}
