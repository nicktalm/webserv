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
		std::string							_clientsMsg;
		std::string							_statusCode;
		std::string							_method;
		std::string							_path;
		std::string							_protocol;
		std::string							_body;
		std::map<std::string, std::string>	_header;
	public:
		Client(void);
		void		appendMsg(char *msg, size_t size);
		void		parseRequest(int fd);
		void		clearMsg(void);

		std::string	getMethod(void) const {return _method;};
		std::string	getstatusCode(void) {return _statusCode;};
		std::string	getProtocol(void) const {return _protocol;};
		int			getFd(void) const {return _fd;};
};
