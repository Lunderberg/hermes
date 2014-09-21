#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <deque>
#include <vector>
#include <memory>

#include <boost/asio.hpp>

#include "Message.hh"

constexpr size_t header_size = sizeof(id_type) + sizeof(size_type);
union network_header{
	struct{
		id_type id;
		size_type size;
	};
	char arr[header_size];
};

class NetworkSocket{
public:
	NetworkSocket(std::shared_ptr<boost::asio::io_service> io_service,
								boost::asio::ip::tcp::resolver::iterator endpoint);
	virtual ~NetworkSocket();
	void write(const Message& message);
private:
	void do_read_header();
	void do_read_body();
	void do_write();

	std::unique_ptr<Message> Unpack(Message::message_id id, std::string data);

	std::shared_ptr<boost::asio::io_service> m_io_service;
	boost::asio::ip::tcp::socket m_socket;

	network_header m_read_header;
	std::vector<char> m_read_body;
	std::deque<std::unique_ptr<Message> > m_read_messages;
	std::deque<std::vector<char> > m_write_messages;
};

#endif /* _NETWORKSOCKET_H_ */
