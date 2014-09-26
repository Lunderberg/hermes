#include "ListenServer.hh"

hermes::ListenServer::ListenServer(std::shared_ptr<hermes::NetworkIO> io,
													 boost::asio::ip::tcp::endpoint endpoint) :
	m_io(io), m_acceptor(*io->GetService(),endpoint), m_socket(*io->GetService()){
	do_accept();
}

void hermes::ListenServer::do_accept(){
	m_acceptor.async_accept(m_socket,
													[this](boost::system::error_code ec){
														if(!ec){
															auto connection = std::make_shared<hermes::NetworkSocket>(m_io,
																																								std::move(m_socket));
															m_connections.push_back(connection);
														}
														do_accept();
													});
}
