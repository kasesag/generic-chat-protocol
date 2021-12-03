#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include "client.h"

// comment this to disable debugging info
#define DEBUG
 
using boost::asio::ip::tcp;

std::string username;

std::string get_msg() {
	std::string msg;
	std::getline(std::cin, msg);
	return msg;
}

void send_data(const std::string &data, tcp::socket *socket) {
   	std::string len(4, 0);
  	
   	for (size_t i = 0; i < 4; i++) {
   		len[i] = (data.size() >> (i * 8)) & 0xFF;
   	}
    
   	std::string full = len + data;
    
   	boost::system::error_code ignored_error;
   	boost::asio::write(*socket, boost::asio::buffer(full), ignored_error);
   	if (ignored_error) {
   		throw boost::system::system_error(ignored_error);
   	}
    
   	//debug info
	#ifdef DEBUG
   	printf("[D!] Sent %zu bytes! Contents: \"%s\" \n", data.size(), data.c_str());
	#endif
}
 
void recv_worker(tcp::socket *socket) {
	boost::system::error_code err;
	std::ostringstream tmp_str;
	std::array<char, 128> buf;
	std::array<uint8_t, 4> size_buf{0,0,0,0};
	uint32_t size;
	char tmp[129];
	
	while(1) {
		tmp_str.str("");
		size_t len = socket->read_some(boost::asio::buffer(size_buf), err);
		if (len != 4) throw std::runtime_error("didn't receive 4 size bytes");
		size = 0;
		for(size_t i = 0; i < 4; i++) {
			size |= ((uint32_t)size_buf[i]) << (i * 8);
		}
 
		size_t recv_len = 0;
		while(1) {
			if (!size) break;
			buf.fill(0);
			memset(tmp, 0, 129);
			
			size_t len = socket->read_some(boost::asio::buffer(buf), err);
			if (err == boost::asio::error::eof) {
				break;
			} else if (err)
				throw boost::system::system_error(err);
			
			recv_len += len;
			std::copy(std::begin(buf), std::begin(buf) + len, std::begin(tmp));
			tmp_str << tmp;
			if (recv_len == size) break;
		}
		
		std::string msg = tmp_str.str();
	
		nlohmann::json j = nlohmann::json::parse(msg);
 
		if (j.find("type") == j.end()) {
			continue; // no type, discard
		}
 
		message_type t = (message_type)j["type"].get<int>();
		if (t == message_type::message_out) {
			printf("<%s>: %s\n", j["username"].get<std::string>().c_str(), j["content"].get<std::string>().c_str());
		}
	}
}

void push_msg(tcp::socket *socket) {
	std::string content = get_msg();
 
	nlohmann::json msg = {
		{"type", (int)message_type::message_in},
		{"content", content}
	};
 
	send_data(msg.dump(), socket);
}
 
int main(int argc, char* argv[]) {
	printf("Chat protocol - Client\n\n");
	if (argc != 4) {
		std::cerr << "Usage: client <host> <port> <nickname>" << std::endl;
		return 1;
    }
 
    username = argv[3];
    printf("[I!] Your username is: %s\n", username.c_str());
 
    boost::asio::io_context io_context;
 
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], argv[2]);
 
    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);
 
	nlohmann::json j = {
		{"type", (int)message_type::username},
		{"username", username}
	};
 
    send_data(j.dump(), &socket);
 
	std::thread *tr = new std::thread(recv_worker, &socket);
 
	while(1) {
		push_msg(&socket);
	}
	
	return 0;
}