#include "NetworkIO.hh"

#include <stdexcept> // for std::runtime_error
#include <algorithm> // for std::copy
#include <iterator> // for std::back_insertor

#include "NetworkSocket.hh"
#include "ListenServer.hh"

using boost::asio::ip::tcp;

std::shared_ptr<NetworkIO> NetworkIO::start(){
	//Can't use std::make_shared because the contstructor is private
	return std::shared_ptr<NetworkIO>(new NetworkIO);
}

NetworkIO::NetworkIO()
	: m_io_service(new boost::asio::io_service), m_work(*m_io_service), m_thread() {
	m_thread = std::thread( [this](){
			while(true){
				try{
					m_io_service->run();
				} catch (std::exception& e) {
					continue;
				}
				break;
			}
		});
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
	return std::make_shared<NetworkSocket>(shared_from_this(), endpoint);
}

std::shared_ptr<ListenServer> NetworkIO::listen(int port){
	tcp::endpoint endpoint(tcp::v4(), port);
	return std::make_shared<ListenServer>(shared_from_this(), endpoint);
}
