#ifndef _RAWTEXTMESSAGE_H_
#define _RAWTEXTMESSAGE_H_

#include <string>
#include <memory>
#include <sstream>

#include "Message.hh"

class RawTextMessage : public Message{
public:
	RawTextMessage(){}
	RawTextMessage(std::string text) : m_text(text) {}
	std::string GetText(){return m_text;}

private:
	std::string m_text;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version){
		ar & m_text;
	}
	MESSAGE_CLASS(RawTextMessage);
};

#endif /* _RAWTEXTMESSAGE_H_ */
