#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <dirent.h>

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
		std::string							_responseBuffer;
		ssize_t								_bytesSent;
	public:
		Client(void) {};
		Client(int fd, const std::string &msg);
		std::string getMethod(void) const {return _method;};
		std::string getstatusCode(void) {return _statusCode;};
		std::string getProtocol(void) const {return _protocol;};
		std::string getFd(void) const {return std::to_string(_fd);};
		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		std::string	getResponseBuffer(void) {return _responseBuffer;};
		ssize_t		&getBytesSent(void) { return _bytesSent;};
};
