#include "NetworkIO.hh"

#include <stdexcept> // for std::runtime_error
#include <algorithm> // for std::copy
#include <iterator> // for std::back_insertor

#include "NetworkSocket.hh"

using boost::asio::ip::tcp;

NetworkIO::NetworkIO()
	: m_io_service(new boost::asio::io_service), m_work(*m_io_service), m_thread() {
	m_thread = std::thread( [this](){ m_io_service->run(); } );
	//The work object dies with the NetworkIO object,
	//  allowing the io_service to end once all async operations end.
	m_thread.detach();
}

std::shared_ptr<NetworkSocket> NetworkIO::connect(std::string server, int port){
	return connect(server, std::to_string(port));
}

std::shared_ptr<NetworkSocket> NetworkIO::connect(std::string server, std::string port){
	tcp::resolver resolver(*m_io_service);
	auto endpoint = resolver.resolve({server,port});
	return std::make_shared<NetworkSocket>(m_io_service, endpoint);
}

std::shared_ptr<ListenServer> NetworkIO::listen(int port){
	tcp::endpoint endpoint(tcp::v4(), port);
	return std::make_shared<ListenServer>(m_io_service, endpoint);
}
