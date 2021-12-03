#include <cstdio>
#include <boost/asio.hpp>
#include <thread>
#include <queue>
#include <string>
#include <random>
#include <nlohmann/json.hpp>
//#include <mutex>
#include "server.h"

// comment this to disable debugging info
#define DEBUG

using boost::asio::ip::tcp;

std::map<int, std::thread *> threads;
std::map<int, tcp::socket *> sockets;
std::map<int, std::string> nicknames;

std::mutex client_maps_mutex; 

std::vector<int> discarded_keys;
std::mutex discarded_keys_mutex;

void delete_threads() {
	while(1) {
		std::lock_guard<std::mutex> g1(discarded_keys_mutex);
		std::lock_guard<std::mutex> g2(client_maps_mutex);
		for (auto key : discarded_keys) {
			threads[key]->join();
 
			delete sockets[key];
			sockets.erase(key);
 
			delete threads[key];
			threads.erase(key);
			nicknames.erase(key);
		}
		discarded_keys.clear();
	}
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
}
 
// sends message to all connected clients
void broadcast_data(const std::string &data) {
	for (auto &c : sockets)
		send_data(data, c.second);
}
 
void recv_worker(tcp::socket *socket, int key) {
	boost::system::error_code err;
	std::ostringstream tmp_str;
	std::array<char, 128> buf;
	std::array<uint8_t, 4> size_buf{0,0,0,0};
	uint32_t size;
	char tmp[129];
	while(1) {
		tmp_str.str("");
		size_t len = socket->read_some(boost::asio::buffer(size_buf), err);
		if (err == boost::asio::error::broken_pipe ||
			err == boost::asio::error::eof || err == boost::asio::error::connection_aborted ||
			err == boost::asio::error::connection_reset) {
			std::lock_guard<std::mutex> g(discarded_keys_mutex);
			discarded_keys.push_back(key);
			return;
		}
 
		if (len != 4) throw std::runtime_error("didn't receive 4 size bytes");
		size = 0;
		for(size_t i = 0; i < 4; i++) {
			size |= ((uint32_t)size_buf[i]) << (i * 8);
		}
 
		#ifdef DEBUG
		printf("Expecting to receive %zu bytes\n", size);
		#endif
			
		size_t recv_len = 0;
		while(1) {
			if (!size) break;
			buf.fill(0);
			memset(tmp, 0, 129);
			
			size_t len = socket->read_some(boost::asio::buffer(buf), err);
			if (err == boost::asio::error::broken_pipe || err == boost::asio::error::eof ||
			err == boost::asio::error::connection_aborted || err == boost::asio::error::connection_reset) {
				std::lock_guard<std::mutex> g(discarded_keys_mutex);
				discarded_keys.push_back(key);
			} 
			else if (err)
				throw boost::system::system_error(err);
			
			recv_len += len;
			std::copy(std::begin(buf), std::begin(buf) + len, std::begin(tmp));
			tmp_str << tmp;
			#ifdef DEBUG
			printf("Received %zu bytes from client, content: '%s'\n", len, tmp);
			#endif
			if (recv_len == size) break;
		}
 
		std::string msg = tmp_str.str();
 
		nlohmann::json j = nlohmann::json::parse(msg);
		if (j.find("type") == j.end()) {
			continue; // no type, discard
		}
 
		message_type t = (message_type)j["type"].get<int>();
 
		if (t == message_type::username) {
			std::string username = j["username"].get<std::string>();
			nicknames.emplace(key, username);
		}
 
		if (t == message_type::message_in) {
			// send message to other users, append client's name
			nlohmann::json in = nlohmann::json::parse(msg);
 
			nlohmann::json jmsg = {
				{"type", (int)message_type::message_out},
				{"username", nicknames[key]},
				{"content", in["content"]}
			};
			#ifdef DEBUG
			printf("[!] broadcasting %s\n", jmsg.dump().c_str());
			#endif
			broadcast_data(jmsg.dump());
		}
	}
}

int main() {
	printf("Chat protocol - server\n\n");

	int port;
	port = 6660;
 
	boost::asio::io_context io_context;
	tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
 
	std::random_device rd;
	std::mt19937 rnd(rd());
 
	printf("Created socket on port %d \n", port);
 
	std::thread discard_worker(delete_threads);
 
	while(1) {
		tcp::socket *sock = new tcp::socket(io_context);
		acceptor.accept(*sock);
 
		int key = rnd();
 
		std::thread *tr = new std::thread(recv_worker, sock, key);
 
		sockets.emplace(key, sock);
		threads.emplace(key, tr);
 
		printf("New client connected.\n");
	}
 
	for (auto t : threads) delete t.second;
	for (auto s : sockets) delete s.second;
 
	return 0;
}
