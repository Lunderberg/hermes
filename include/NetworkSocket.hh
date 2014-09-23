#ifndef _NETWORKSOCKET_H_
#define _NETWORKSOCKET_H_

#include <deque>
#include <vector>
#include <memory>
#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

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
	NetworkSocket(std::shared_ptr<boost::asio::io_service> io_service,
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

	std::shared_ptr<Message> Unpack(id_type id, std::string data);

	std::shared_ptr<boost::asio::io_service> m_io_service;
	boost::asio::ip::tcp::socket m_socket;

	network_header m_read_header;
	std::vector<char> m_read_body;
	std::deque<std::shared_ptr<Message> > m_read_messages;
	std::deque<std::vector<char> > m_write_messages;
};

#define NETWORK_ENUM_ID_ELEM(_,__,cls) cls,

#define NETWORK_ENUM_ID(class_list)													\
	enum class message_id : id_type {													\
		BOOST_PP_SEQ_FOR_EACH(NETWORK_ENUM_ID_ELEM,,class_list) \
	};

#define NETWORK_GETID_ELEM(_,__,cls)						\
	id_type cls::GetID() const {									\
		return id_type(message_id::cls);						\
	}

#define NETWORK_GETID_ALL(class_list) \
	BOOST_PP_SEQ_FOR_EACH(NETWORK_GETID_ELEM,,class_list)

#define NETWORK_UNPACK_CASE(_,__,cls)								\
	case message_id::cls:															\
	{																									\
		auto out = std::make_shared<cls>();							\
		out->Unpack(data);															\
		return out;																			\
	}

#define NETWORK_UNPACK(class_list)																			\
	NETWORK_ENUM_ID(class_list)																						\
	NETWORK_GETID_ALL(class_list)																					\
	std::shared_ptr<Message> NetworkSocket::Unpack(id_type id,						\
																								 std::string data){			\
		message_id msg_id = static_cast<message_id>(id);										\
		switch(msg_id){																											\
			BOOST_PP_SEQ_FOR_EACH(NETWORK_UNPACK_CASE,,class_list)						\
		default:																														\
			throw std::runtime_error("Unknown packet id");										\
		}																																		\
	}

#endif /* _NETWORKSOCKET_H_ */
