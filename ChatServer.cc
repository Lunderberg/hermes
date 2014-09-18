#include <deque>
#include <memory>
#include <set>
#include <iostream>
#include <list>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include "ChatMessage.hh"

typedef std::deque<ChatMessage> chat_message_queue;

class ChatParticipant{
public:
	virtual ~ChatParticipant() {}
	virtual void deliver(const ChatMessage& msg) = 0;
};

typedef std::shared_ptr<ChatParticipant> ChatParticipant_ptr;

class ChatRoom{
public:
	void join(ChatParticipant_ptr participant){
		m_participants.insert(participant);
		for (auto msg: m_recent_msgs){
			participant->deliver(msg);
		}
	}

	void leave(ChatParticipant_ptr participant){
		m_participants.erase(participant);
	}

	void deliver(const ChatMessage& msg){
		m_recent_msgs.push_back(msg);
		while(m_recent_msgs.size() > max_recent_msgs){
			m_recent_msgs.pop_front();
		}

		for(auto participant : m_participants){
			participant->deliver(msg);
		}
	}
private:
	std::set<ChatParticipant_ptr> m_participants;
	enum { max_recent_msgs = 100 };
	chat_message_queue m_recent_msgs;
};

class ChatSession :
	public ChatParticipant,
	public std::enable_shared_from_this<ChatSession>{
public:
	ChatSession(tcp::socket socket, ChatRoom& room) :
		m_socket(std::move(socket)), m_room(room) { }

	void start(){
		m_room.join(shared_from_this());
		do_read_header();
	}

	void deliver(const ChatMessage& msg){
		bool write_in_progress = !m_write_msgs.empty();
		m_write_msgs.push_back(msg);
		if(!write_in_progress){
			do_write();
		}
	}

private:
	void do_read_header(){
		auto self(shared_from_this());
		boost::asio::async_read(m_socket,
														boost::asio::buffer(m_read_msg.data(), ChatMessage::header_length),
														[this,self](boost::system::error_code ec, std::size_t length){
															if(!ec && m_read_msg.decode_header()){
																do_read_body();
															} else {
																m_room.leave(shared_from_this());
															}
														});
	}

	void do_read_body(){
		auto self(shared_from_this());
		boost::asio::async_write(m_socket,
														 boost::asio::buffer(m_write_msgs.front().data(),m_write_msgs.front().length()),
														 [this,self](boost::system::error_code ec, std::size_t length){
															 if(!ec){
																 m_write_msgs.pop_front();
																 if(!m_write_msgs.empty()){
																	 do_write();
																 }
															 } else {
																 m_room.leave(shared_from_this());
															 }
														 });
	}

	void do_write(){
		auto self(shared_from_this());
		boost::asio::async_write(m_socket,
														 boost::asio::buffer(m_write_msgs.front().data(),m_write_msgs.front().length()),
														 [this,self](boost::system::error_code ec, std::size_t length){
															 if(!ec){
																 m_write_msgs.pop_front();
																 if(!m_write_msgs.empty()){
																	 do_write();
																 }
															 } else {
																 m_room.leave(shared_from_this());
															 }
														 });
	}


	tcp::socket m_socket;
	ChatRoom& m_room;
	ChatMessage m_read_msg;
	chat_message_queue m_write_msgs;
};

class ChatServer{
public:
	ChatServer(boost::asio::io_service& io_service, const tcp::endpoint& endpoint) :
		m_acceptor(io_service,endpoint), m_socket(io_service) {
		do_accept();
	}
private:
	void do_accept(){
		m_acceptor.async_accept(m_socket,
														[this](boost::system::error_code ec){
															if(!ec){
																std::make_shared<ChatSession>(std::move(m_socket),m_room)->start();
															}
															do_accept();
														});
	}

	tcp::acceptor m_acceptor;
	tcp::socket m_socket;
	ChatRoom m_room;
};

int main(int argc, char* argv[]){
	try{
		if(argc<2){
			std::cerr << "Usage: ChatServer <port> [<port> ..]" << std::endl;
			return 1;
		}

		boost::asio::io_service io_service;

		std::list<ChatServer> servers;
		for(int i=1; i<argc; i++){
			tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
			servers.emplace_back(io_service, endpoint);
		}

		io_service.run();
	} catch (std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}
