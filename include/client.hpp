#pragma once

#include <vector>
#include <map>
#include <string>
#include <ctime>
#include "config.hpp"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

class Client
{
	private:
		bool								_listen;
		bool								_headerReady;
		int									_fd;
		std::string							_clientsMsg;
		std::string							_statusCode;
		std::string							_method;
		std::string							_path;
		std::string							_protocol;
		std::string							_body;
		std::map<std::string, std::string>	_header;
		std::string							_autoIndexBody;
		std::string							_reDirHeader;
		std::string							_responseBuffer;
		ssize_t								_bytesSent;
	public:
		Client(void);
		~Client(void);
		void		appendMsg(char *msg, size_t size);
		void		parseRequest(int fd, const t_config config);
		void		createAutoIndex(const std::string &lastDir);
		void		checkFile(const std::string &lastDir, const std::string &file);
		bool		splitPath(std::string &fullpath, std::string &fiirstDir, std::string &file);
		void		checkBodySize(std::stringstream &parse);
		void		headerParsing(int fd, const t_config config, std::stringstream &parse);

		std::string	getAutoIndex(void) const {return (_autoIndexBody);};
		bool		getListen(void) const {return (_listen);};
		std::string	getReDir(void) const {return (_reDirHeader);};
		std::string	getMethod(void) const {return (_method);};
		std::string	&getMsg(void) {return (_clientsMsg);};
		std::string	getstatusCode(void) {return _statusCode;};
		std::string	getProtocol(void) const {return _protocol;};
		std::string	getPath(void);
		std::string	&getBody(void) {return _body;};
		std::string setBody(const std::string &body) {_body = body; return _body;};
		int			getFd(void) const {return _fd;};
		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		std::string	&getResponseBuffer(void) {return _responseBuffer;};
		ssize_t		&getBytesSent(void) { return _bytesSent;};
		void		checkPath(const t_config config);
		bool		checkLocation(const t_config config, const std::string &firstDir, std::string &lastDir, std::string &file, bool &autoindex, bool &reDir);
		void		setStatusCode(std::string code) {_statusCode = code;};
};

std::string	getSize(off_t &bytes);
std::string	getTime(std::time_t time);