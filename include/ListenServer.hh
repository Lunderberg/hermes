#ifndef _LISTENSERVER_H_
#define _LISTENSERVER_H_

#include <memory>
#include <deque>

#include <boost/asio.hpp>

#include "NetworkSocket.hh"

using boost::asio::ip::tcp;

class ListenServer{
public:
	ListenServer(std::shared_ptr<boost::asio::io_service> io_service,
							 tcp::endpoint endpoint);
	bool HasNewConnection(){return m_connections.size();}
	std::shared_ptr<NetworkSocket> GetConnection(){
		auto output = m_connections.front();
		m_connections.pop_front();
		return output;
	}
private:
	void do_accept();

	std::shared_ptr<boost::asio::io_service> m_io_service;
	tcp::acceptor m_acceptor;
	tcp::socket m_socket;
	std::deque<std::shared_ptr<NetworkSocket> > m_connections;
};

#endif /* _LISTENSERVER_H_ */
