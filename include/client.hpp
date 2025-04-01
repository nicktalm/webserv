#pragma once

#include <vector>
#include <map>
#include <string>
#include "config.hpp"

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
		std::string							_responseBuffer;
		ssize_t								_bytesSent;
	public:
		Client(void);
		~Client(void);
		void		appendMsg(char *msg, size_t size);
		void		parseRequest(int fd);

		std::string	getMethod(void) const {return (_method);};
		std::string	getMsg(void) {return (_clientsMsg);};
		std::string	getstatusCode(void) {return _statusCode;};
		std::string	getProtocol(void) const {return _protocol;};
		std::string	getPath(const t_config &config);
		int			getFd(void) const {return _fd;};
		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		std::string	getResponseBuffer(void) {return _responseBuffer;};
		ssize_t		&getBytesSent(void) { return _bytesSent;};
		bool		checkPath(const t_config config, std::string &dir, std::string &file);
};
