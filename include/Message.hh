#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdint>
#include <string>
#include <memory>

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

class Message{
public:
	enum class message_id : id_type {RawTextMessage};
	virtual void Unpack(const std::string& raw) = 0;
	virtual std::string Pack() const = 0;
	virtual id_type GetID() const = 0;
};

#define MESSAGE_CLASS(cls)																	\
	friend class boost::serialization::access;								\
	public:																										\
	enum class id : id_type {id = Message::message_id::cls};	\
	virtual id_type GetID() const { return id_type(id::id); }	\
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

#endif /* _MESSAGE_H_ */
