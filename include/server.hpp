#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"


class Server
{
	private:
		sockaddr_in	addr;
		int	socketFd;
		int	client;
		int	port;

	public:
		~Server(void);

		int		get_socketfd(void) {return socketFd;};
		int		get_port(void) {return port;};
		void	initServer(void);
		void	request(void);
		void	response(void);
};

// reads file and return the input as an const std::string
std::string	readFile(std::string input);

//gerne anpassen, ich glaube wir brauchen die values aber nicht sicher