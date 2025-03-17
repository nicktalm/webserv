#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"


class Server
{
	private:
		int socketfd;
		int port;

	public:
		int get_socketfd(void) {return socketfd;};
		int get_port(void) {return port;};
		void initServer(void);
};

//gerne anpassen, ich glaube wir brauchen die values aber nicht sicher