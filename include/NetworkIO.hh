#ifndef _NETWORKIO_H_
#define _NETWORKIO_H_

#include <thread>
#include <string>
#include <memory>
#include <deque>

#include <boost/asio.hpp>

#include "NetworkSocket.hh"
#include "Message.hh"


class NetworkIO{
public:
	NetworkIO();
	std::shared_ptr<NetworkSocket> connect(std::string server, int port);
	std::shared_ptr<NetworkSocket> connect(std::string server, std::string port);

private:
	std::shared_ptr<boost::asio::io_service> m_io_service;
	boost::asio::io_service::work m_work;
	std::thread m_thread;
};

#endif /* _NETWORKIO_H_ */
