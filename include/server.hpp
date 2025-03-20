#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <poll.h>
#include <atomic>
#include <vector>
#include <cerrno>
// #include "client.hpp"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

typedef struct s_location
{

}	t_location;

typedef struct s_config
{
	int			port;
	std::string	server_name;
	t_location	location;
}	t_config;

class Server
{
	private:
		sockaddr_in			_addr;
		std::vector<pollfd>	_clientsInfo;
		// std::vector<Client>	_clients;
		const t_config		_config;
		int					_socketFd;

	public:
		Server(t_config config);
		~Server(void);

		int		get_socketfd(void) {return _socketFd;};
		int		get_port(void) {return _config.port;};
		void	request(void);
		void	run(void);
		// void	response(void);
};


// reads file and return the input as an const std::string
std::string	readFile(std::string input);

//gerne anpassen, ich glaube wir brauchen die values aber nicht sicher