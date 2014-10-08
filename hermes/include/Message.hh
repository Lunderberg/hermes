#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdint>
#include <string>
#include <memory>
#include <sstream>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>

namespace hermes{

	typedef boost::archive::text_oarchive oarchive;
	typedef boost::archive::text_iarchive iarchive;
	// typedef boost::archive::binary_oarchive oarchive;
	// typedef boost::archive::binary_iarchive iarchive;

	typedef std::int16_t id_type;
	typedef std::uint16_t size_type;
	constexpr size_type max_message_size = UINT16_MAX;

	constexpr size_t header_size = sizeof(id_type) + sizeof(size_type) + sizeof(char);
	union network_header{
		struct{
			id_type id;
			size_type size;
			char acknowledge;
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

}

#define HERMES_MESSAGE_CLASS(cls)								\
	friend class boost::serialization::access;		\
public:																					\
 virtual hermes::id_type GetID() const;					\
 virtual void Unpack(const std::string& raw){		\
	 std::stringstream ss;												\
	 ss.str(raw);																	\
	 {																						\
		 hermes::iarchive ia(ss);										\
		 ia >> (*this);															\
	 }																						\
 }																							\
 virtual std::string Pack() const {							\
	 std::stringstream ss;												\
	 {																						\
		 hermes::oarchive io(ss);										\
		 io << (*this);															\
	 }																						\
	 return ss.str();															\
 }

#define HERMES_NETWORK_ENUM_ID_ELEM(_,__,cls) cls,

#define HERMES_NETWORK_ENUM_ID(class_list)													\
	enum class message_id : hermes::id_type {													\
		BOOST_PP_SEQ_FOR_EACH(HERMES_NETWORK_ENUM_ID_ELEM,,class_list)	\
	};

#define HERMES_NETWORK_GETID_ELEM(_,__,cls)			\
	hermes::id_type cls::GetID() const {					\
		return hermes::id_type(message_id::cls);		\
	}

#define HERMES_NETWORK_GETID_ALL(class_list)										\
	BOOST_PP_SEQ_FOR_EACH(HERMES_NETWORK_GETID_ELEM,,class_list)

#define HERMES_NETWORK_UNPACK_CASE(_,__,cls)		\
	case message_id::cls:													\
	{																							\
		auto out = std::make_shared<cls>();					\
		out->Unpack(data);													\
		return out;																	\
	}

#define HERMES_NETWORK_UNPACK(class_list)																\
	HERMES_NETWORK_ENUM_ID(class_list)																		\
	HERMES_NETWORK_GETID_ALL(class_list)																	\
		std::shared_ptr<hermes::Message> hermes::Message::Unpack(hermes::id_type id, \
																														 std::string data){	\
		message_id msg_id = static_cast<message_id>(id);										\
		switch(msg_id){																											\
			BOOST_PP_SEQ_FOR_EACH(HERMES_NETWORK_UNPACK_CASE,,class_list)			\
		default:																														\
				throw std::runtime_error("Unknown packet id");									\
		}																																		\
	}

#define HERMES_MESSAGE_TYPES(...)																\
	HERMES_NETWORK_UNPACK(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#endif /* _MESSAGE_H_ */
