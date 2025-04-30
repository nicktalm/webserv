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

		int			getSocketfd(void) {return _socketFd;};
		int			getPort(void) {return _config.port;};
		std::string	getRoot(void) {return (_config.root);};
		void		request(std::vector<pollfd>::iterator clientFd);
		void		run(void);
		void		response(Client &client, std::vector<pollfd>::iterator pollClient);
		void		IO_Error(int bytesRead, std::vector<pollfd>::iterator find);
		std::string	handleGET(Client &client);
		std::string	handlePOST(Client &client);
		bool		execute_cgi(Client &client);
		void		createEnv(Client &client, std::vector<std::string> &envpStrings, std::vector<char *> &envs);
		void		childProcess(Client &client, int *pipeIn, int *pipeOut);
		bool		parentProcess(Client &client, int *pipeIn, int *pipeOut);
		bool		waitingroom(Client &client);
		std::string	readFromFd(Client &client);
		std::string	checkCGIOutput(Client &client, char *buffer);
		std::string	checkCGI(Client &client);

		std::string	extractContentType(const std::string &headerValue);
		bool		handleMultipartFormData(Client &client, const std::string &body, const std::string &uploadDir);
		bool		handleURLEncoded(Client &client, const std::string &body, const std::string &uploadDir);
		bool		handleRawUpload(Client &client, const std::string &body, const std::string &uploadDir);
		std::string	buildRedirectResponse(const std::string &location);

		std::string	handleERROR(Client &client);
		std::string	handleDELETE(Client &client);
		std::string	create_response(const t_response &response);
		void		disconnect(std::vector<pollfd>::iterator find);
};

// checks the config file and returns a vector of t_config
std::string							trim(const std::string& line);
std::string							to_lower(const std::string &str);
bool								check_config(const std::string &config_path, std::vector<t_config> &files);
void								add_default_location(std::vector<t_config> &files);
bool								process_server_block(const std::string &line, t_config &current_config, bool &in_server_block, bool &expect_server_brace);
bool								process_location_block(const std::string &line, t_location &current_location, bool &in_location_block, bool &expect_location_brace);
bool								process_closing_brace(const std::string& line, t_config& current_config, t_location& current_location, std::vector<t_config>& files, bool& in_server_block, bool& in_location_block);
void								process_location_directives(const std::string& line, t_location& current_location);
void								process_server_directives(const std::string& line, t_config& current_config);
void								process_client_max_body_size(std::istringstream& iss, long& max_size, char& unit);
void								debug_parsed_configurations(const std::vector<t_config>& files);
std::map<std::string, std::string>	parseBody(std::string body);