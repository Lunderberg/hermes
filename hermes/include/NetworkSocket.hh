#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <deque>
#include <vector>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <mutex>

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

	bool HasNewMessage();
	bool IsOpen();
	std::shared_ptr<Message> GetMessage();
	bool SendInProgress();
	int WriteMessagesQueued();

protected:
	void do_read_header();
	void do_read_body();
	void do_write();

	std::shared_ptr<NetworkIO> m_io;
	boost::asio::ip::tcp::socket m_socket;

	network_header m_read_header;
	std::vector<char> m_read_body;
	std::deque<std::shared_ptr<Message> > m_read_messages;
	std::recursive_mutex m_read_lock;

	std::deque<std::vector<char> > m_write_messages;
	std::vector<char> m_current_write;
	std::atomic_bool m_is_writing;
	std::atomic_bool m_writer_running;
	std::recursive_mutex m_write_lock;
};



#endif /* _NETWORKSOCKET_H_ */
