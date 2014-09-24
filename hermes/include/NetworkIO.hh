#ifndef _NETWORKIO_H_
#define _NETWORKIO_H_

#include <thread>
#include <string>
#include <memory>
#include <deque>

#include <iostream>

#include <boost/asio.hpp>

#include "Message.hh"
#include "ListenServer.hh"
#include "NetworkSocket.hh"

class NetworkSocket;
class ListenServer;

class NetworkIO : public std::enable_shared_from_this<NetworkIO> {
public:
	static std::shared_ptr<NetworkIO> start();
	std::shared_ptr<NetworkSocket> connect(std::string server, int port);
	std::shared_ptr<NetworkSocket> connect(std::string server, std::string port);
	std::shared_ptr<ListenServer> listen(int port);

	std::shared_ptr<boost::asio::io_service> GetService(){return m_io_service;}
	~NetworkIO();

private:
	NetworkIO();

	std::thread m_thread;
	std::shared_ptr<boost::asio::io_service> m_io_service;
	boost::asio::io_service::work m_work;
};

#endif /* _NETWORKIO_H_ */
