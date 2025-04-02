#pragma once

#include <poll.h>
#include <vector>
#include <map>
#include <string>
#include <atomic>
#include "config.hpp"
#include "client.hpp"
#include "response.hpp"

extern std::atomic<bool>	runner;

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define PURPLE  "\033[35m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"
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
		std::string 	handlePOST(Client &client);
		std::string		handleERROR(Client &client);
		std::string 	create_response(const t_response &response);
		void			disconnect(std::vector<pollfd>::iterator find);
};

// checks the config file and returns a vector of t_config
bool		check_config(const std::string& config_path, std::vector<t_config>& files);
