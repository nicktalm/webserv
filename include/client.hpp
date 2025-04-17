#pragma once

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <ctime>
#include "config.hpp"
#include "response.hpp"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

class Client : public Response
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
		t_location							_locationInfo;
		std::map<std::string, std::string>	_header;
	public:
		Client(void);
		~Client(void);
		void		appendMsg(char *msg, size_t size);
		void		parseRequest(int fd, const t_config config);
		std::string	createAutoIndex(const std::string &lastDir, const std::string name);
		void		checkFile(std::string &fullDir, std::string file);
		bool		splitPath(std::string &fullpath, std::string &fiirstDir, std::string &file);
		void		checkBodySize(void);
		bool		findLocation(const t_config config, std::string fullDir);
		bool		checkLocation(std::string &fullDir, const std::string &mainRoot);
		bool		checkBodyLimit(const long rootMaxSize);
		void		headerParsing(int fd, const t_config config);
		void		checkPath(const t_config config);
		
		// getter
		bool								getListen(void) const {return (_listen);};
		int									getFd(void) const {return _fd;};
		std::string							getReDir(void) const {return (_reDirHeader);};
		std::string							getMethod(void) const {return (_method);};
		std::string							getMsg(void) {return (_clientsMsg);};
		std::string							getStatusCode(void) {return _statusCode;};
		std::string							getProtocol(void) const {return _protocol;};
		std::string							getPath(void);
		std::string							getBody(void) {return _body;};
		std::map<std::string, std::string>	getHeader(void) const {return _header;};
		
		// setter
		void	setBody(const std::string &body) {_body = body;};
		void	setStatusCode(std::string code) {_statusCode = code;};
};

std::string	getSize(off_t &bytes);
std::string	getTime(std::time_t time);