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
#include <csignal>
#include <netdb.h>
#include <ctime>
#include <string>

#include "client.hpp"
#include "utils.hpp"
#include "response.hpp"

extern std::atomic<bool>	runner;

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define PURPLE  "\033[35m"
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
	std::vector<t_location> locations;
}	t_config;

typedef struct s_response
{
	std::string	start_line = "";
	std::string	date = "";
	std::string	content_type = "";
	std::string	content_length = "";
	std::string empty_line = "\r\n";
	std::string	body = "";
	std::string server_name = "";
}	t_response;


class Server
{
	private:
		std::vector<pollfd>			_clientsFd;
		std::map<int, Client>		_clientsInfo;
		const t_config				_config;
		int							_socketFd;
		struct addrinfo 			*_res;

	public:
		Server(t_config config);
		~Server(void);

		int				getSocketfd(void) {return _socketFd;};
		int				getPort(void) {return _config.port;};
		std::string		getRoot(void) {return (_config.root);};
		void			request(std::vector<pollfd>::iterator clientFd);
		void			run(void);
		void			response(Client &client, std::vector<pollfd>::iterator pollClient);
		void			IO_Error(int bytesRead, std::vector<pollfd>::iterator find);
		std::string 	handleGET(Client &client);
		std::string		handleERROR(Client &client);
		std::string 	create_response(t_response response);
};

// checks the config file and returns a vector of t_config
bool		check_config(const std::string& config_path, std::vector<t_config>& files);

//gerne anpassen, ich glaube wir brauchen die values aber nicht sicher