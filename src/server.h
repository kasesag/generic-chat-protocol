#pragma once

#include <cstdio>
#include <boost/asio.hpp>

enum class message_type {
	username = 1,
	message_in,
	message_out
};

void delete_threads();
void send_data(const std::string &data, boost::asio::ip::tcp::socket *socket);
void broadcast_data(const std::string &data);
void recv_worker(boost::asio::ip::tcp::socket *socket, int key);
