#pragma once

#include <cstdio>
//#include <boost/array.hpp>
#include <boost/asio.hpp>

enum class message_type {
	username = 1,
	message_in,
	message_out
};

std::string get_msg();
void send_data(const std::string &data, boost::asio::ip::tcp::socket *socket);
void recv_worker(boost::asio::ip::tcp::socket *socket);
void push_msg(boost::asio::ip::tcp::socket *socket);
