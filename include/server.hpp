#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <poll.h>
#include <atomic>
#include <vector>
#include <cerrno>
#include <map>

#include "client.hpp"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

typedef struct s_location
{
	bool autoindex = false;
	std::string index = "";
	std::string root = "";
	std::multimap<std::string, std::string> error_page = {};
}	t_location;

typedef struct s_config
{
	int port = 0;
	std::string server_name = "";
	std::string index = "";
	std::string root = "";
	t_location location;
}	t_config;


class Server
{
	private:
		sockaddr_in					_addr;
		std::vector<pollfd>			_clientsFd;
		std::map<int, std::string>	_clientsMsg;
		std::vector<Client>			_clientsInfo;
		const t_config				_config;
		int							_socketFd;

	public:
		Server(t_config config);
		~Server(void);

		int		get_socketfd(void) {return _socketFd;};
		int		get_port(void) {return _config.port;};
		void	request(int fd);
		void	run(void);
		void	response(int fd);
};


// reads file and return the input as an const std::string
std::string	readFile(std::string input);

// checks the config file and returns a vector of t_config
bool		check_config(std::string config_path, std::vector<t_config> &files);

//gerne anpassen, ich glaube wir brauchen die values aber nicht sicher