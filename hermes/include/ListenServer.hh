#ifndef _LISTENSERVER_H_
#define _LISTENSERVER_H_

#include <memory>
#include <deque>

#include <boost/asio.hpp>

#include "NetworkIO.hh"

class NetworkIO;
class NetworkSocket;

class ListenServer{
public:
	ListenServer(std::shared_ptr<NetworkIO> io,
							 boost::asio::ip::tcp::endpoint endpoint);
	bool HasNewConnection(){return m_connections.size();}
	std::shared_ptr<NetworkSocket> GetConnection(){
		auto output = m_connections.front();
		m_connections.pop_front();
		return output;
	}
private:
	void do_accept();

	std::shared_ptr<NetworkIO> m_io;
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_socket;
	std::deque<std::shared_ptr<NetworkSocket> > m_connections;
};

#endif /* _LISTENSERVER_H_ */
