#pragma once

#include <iostream>
#include <map>
#include <sstream>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

class Client
{
	private:
		int									_fd;
		std::string							_statusCode;
		std::string							_method;
		std::string							_path;
		std::string							_protocol;
		std::string							_body;
		std::map<std::string, std::string>	_header;
	public:
		Client(int fd, const std::string &msg);
};