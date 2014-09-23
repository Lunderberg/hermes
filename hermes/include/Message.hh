#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdint>
#include <string>
#include <memory>

#include <boost/preprocessor/seq/for_each.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
typedef boost::archive::text_oarchive oarchive;
typedef boost::archive::text_iarchive iarchive;

// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// typedef boost::archive::binary_oarchive oarchive;
// typedef boost::archive::binary_iarchive iarchive;

typedef std::int16_t id_type;
typedef std::uint16_t size_type;
constexpr size_type max_message_size = UINT16_MAX;

constexpr size_t header_size = sizeof(id_type) + sizeof(size_type);
union network_header{
	struct{
		id_type id;
		size_type size;
	};
	char arr[header_size];
};

class Message{
public:
	virtual void Unpack(const std::string& raw) = 0;
	virtual std::string Pack() const = 0;
	virtual id_type GetID() const = 0;
	static std::shared_ptr<Message> Unpack(id_type id, std::string data);
};

#define MESSAGE_CLASS(cls)																	\
	friend class boost::serialization::access;								\
	public:																										\
	virtual id_type GetID() const;														\
	virtual void Unpack(const std::string& raw){							\
		std::stringstream ss;																		\
		ss.str(raw);																						\
		{																												\
			iarchive ia(ss);																			\
			ia >> (*this);																				\
		}																												\
	}																													\
	virtual std::string Pack() const {												\
		std::stringstream ss;																		\
		{																												\
			oarchive io(ss);																			\
			io << (*this);																				\
		}																												\
		return ss.str();																				\
	}

#define NETWORK_ENUM_ID_ELEM(_,__,cls) cls,

#define NETWORK_ENUM_ID(class_list)													\
	enum class message_id : id_type {													\
		BOOST_PP_SEQ_FOR_EACH(NETWORK_ENUM_ID_ELEM,,class_list) \
	};

#define NETWORK_GETID_ELEM(_,__,cls)						\
	id_type cls::GetID() const {									\
		return id_type(message_id::cls);						\
	}

#define NETWORK_GETID_ALL(class_list)										\
	BOOST_PP_SEQ_FOR_EACH(NETWORK_GETID_ELEM,,class_list)

#define NETWORK_UNPACK_CASE(_,__,cls)						\
	case message_id::cls:													\
	{																							\
		auto out = std::make_shared<cls>();					\
		out->Unpack(data);													\
		return out;																	\
	}

#define NETWORK_UNPACK(class_list)															\
	NETWORK_ENUM_ID(class_list)																		\
	NETWORK_GETID_ALL(class_list)																	\
		std::shared_ptr<Message> Message::Unpack(id_type id,				\
																						 std::string data){	\
		message_id msg_id = static_cast<message_id>(id);						\
		switch(msg_id){																							\
			BOOST_PP_SEQ_FOR_EACH(NETWORK_UNPACK_CASE,,class_list)		\
		default:																										\
				throw std::runtime_error("Unknown packet id");					\
		}																														\
	}

#endif /* _MESSAGE_H_ */
