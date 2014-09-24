#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <deque>
#include <vector>
#include <memory>
#include <stdexcept>

#include <boost/asio.hpp>

#include "Message.hh"

class NetworkIO;

class NetworkSocket{
public:
	NetworkSocket(std::shared_ptr<NetworkIO> io,
								boost::asio::ip::tcp::resolver::iterator endpoint);
	NetworkSocket(std::shared_ptr<NetworkIO> io,
								boost::asio::ip::tcp::socket socket);
	virtual ~NetworkSocket();
	void write(const Message& message);

	bool HasNewMessage(){return m_read_messages.size();}
	std::shared_ptr<Message> GetMessage(){
		auto output = m_read_messages.front();
		m_read_messages.pop_front();
		return output;
	}

protected:
	void do_read_header();
	void do_read_body();
	void do_write();

	std::shared_ptr<NetworkIO> m_io;
	boost::asio::ip::tcp::socket m_socket;

	network_header m_read_header;
	std::vector<char> m_read_body;
	std::deque<std::shared_ptr<Message> > m_read_messages;
	std::deque<std::vector<char> > m_write_messages;
};



#endif /* _NETWORKSOCKET_H_ */
