#include "NetworkIO.hh"

#include <stdexcept> // for std::runtime_error
#include <algorithm> // for std::copy
#include <iterator> // for std::back_insertor

using boost::asio::ip::tcp;

NetworkIO::NetworkIO()
	: m_io_service(new boost::asio::io_service), m_work(*m_io_service), m_thread() {
	m_thread = std::thread( [this](){ m_io_service->run(); } );
}

NetworkIO::~NetworkIO(){
	m_io_service->stop();
	m_thread.join();
}

std::shared_ptr<NetworkSocket> NetworkIO::connect(std::string server, int port){
	return connect(server, std::to_string(port));
}

std::shared_ptr<NetworkSocket> NetworkIO::connect(std::string server, std::string port){
	tcp::resolver resolver(*m_io_service);
	auto endpoint = resolver.resolve({server,port});
	return std::make_shared<NetworkSocket>(m_io_service,endpoint);
}


