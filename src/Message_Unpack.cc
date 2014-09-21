#include "NetworkSocket.hh"

#include <memory>
#include <stdexcept>

#include "RawTextMessage.hh"

std::unique_ptr<Message> NetworkSocket::Unpack(Message::message_id id, std::string data){
	switch(id){
	case Message::message_id::RawTextMessage:
		{
			std::unique_ptr<Message> out(new RawTextMessage);
			out->Unpack(data);
			return out;
		}
	default:
		throw std::runtime_error("Unknown packet id");
	}
}
