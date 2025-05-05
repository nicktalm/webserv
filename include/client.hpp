/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 11:45:31 by lbohm             #+#    #+#             */
/*   Updated: 2025/05/05 15:28:22 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <ctime>
#include <iostream>
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
		bool								_chunked;
		int									_fd;
		std::string							_clientsMsg;
		std::string							_statusCode;
		std::string							_method;
		std::string							_path;
		std::string							_protocol;
		std::string							_body;
		std::string							_query;
		std::string							_exePath;
		std::string							_pathInfo;
		t_location							_locationInfo;
		std::map<std::string, std::string>	_header;
	public:
		Client(void);
		~Client(void);
		void		appendMsg(char *msg, size_t size);
		void		parseRequest(int fd, const t_config config);
		std::string	createAutoIndex(const std::string &lastDir, const std::string name);
		bool		checkFile(std::string &fullDir, std::string file);
		bool		splitPath(std::string &fullpath, std::string &fiirstDir, std::string &file);
		void		checkBodySize(void);
		bool		findLocation(const t_config config, std::string fullDir);
		bool		checkLocation(std::string &fullDir, const std::string &mainRoot);
		bool		checkBodyLimit(const long rootMaxSize);
		void		headerParsing(int fd, const t_config config);
		void		checkPath(const t_config config);
		void		clear(void);
		void		parseChunk(std::string chunk);
		void		urlEncoded(void);
		void		queryStr(void);
		void		pathInfo(void);
		void		printStatus(void);

		template<typename Func, typename... ARGS>
		bool	handleFd(Func func, ARGS... args)
		{
			if (func(args...) == -1)
			{
				std::cerr << RED; perror("fd funktion:"); std::cerr << RESET;
				this->setStatusCode("500");
				return (false);
			}
			return (true);
		}
		
		// getter
		bool								getListen(void) const {return (_listen);};
		int									getFd(void) const {return (_fd);};
		std::string							getReDir(void) const {return (_reDirHeader);};
		std::string							getMethod(void) const {return (_method);};
		std::string							getMsg(void) const {return (_clientsMsg);};
		std::string							getStatusCode(void) {return (_statusCode);};
		std::string							getProtocol(void) const {return (_protocol);};
		std::string							getPath(void) const {return (_path);};
		std::string							getBody(void) const {return (_body);};
		std::string							getQuery(void) const {return (_query);};
		std::string							getExePath(void) const {return (_exePath);};
		std::string							getPathInfo(void) const {return (_pathInfo);};
		std::map<std::string, std::string>	getHeader(void) const {return (_header);};
		t_location							getLocationInfo(void) const {return (_locationInfo);};
		
		// setter
		void	setBody(const std::string &body) {_body = body;};
		void	setStatusCode(std::string code) {_statusCode = code;};
};

std::string	getSize(off_t &bytes);
std::string	getTime(std::time_t time);