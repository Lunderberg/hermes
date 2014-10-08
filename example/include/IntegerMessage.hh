#ifndef _INTEGERMESSAGE_H_
#define _INTEGERMESSAGE_H_

#include "Message.hh"

class IntegerMessage : public hermes::Message{
public:
	IntegerMessage(){}
	IntegerMessage(int value) : m_value(value) {}
	int GetValue(){return m_value;}

private:
	int m_value;

	// Necessary function, defining how the class is serialized.
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version){
		ar & m_value;
	}
	// Magic macro, defines a few necessary boilerplate functions
	HERMES_MESSAGE_CLASS(IntegerMessage);
};

#endif /* _INTEGERMESSAGE_H_ */
