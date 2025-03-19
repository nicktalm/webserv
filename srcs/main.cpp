#include "../include/server.hpp"


bool check_config(std::string config_path)
{
	(void)config_path;
	return true;
}

int main(int argc, char **argv)
{
	if (argc > 2)
	{
		std::cout << RED << "Wrong number of arguments, try [./webserv configuration_file]" << RESET << std::endl;
		return EXIT_FAILURE;
	}
	else
	{
		std::string config_file;
		if (argc == 2)
			config_file = argv[1];
		else
			config_file = "/default/default.conf";
		if (!check_config(config_file))
		{
			std::cout << RED << "Error in config file" << RESET << std::endl;
			return EXIT_FAILURE;
		}
	}
	try
	{
		Server server;
		std::cout << GREEN << "Starting server..." << RESET << std::endl;
		server.initServer(); //Startpunkt fuer alles weitere
		server.request();
		server.response();
		close(server.get_socketfd()); // wann socket closen?
	}
	catch (std::exception &e)
	{
		std::cout << RED << e.what() << RESET << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}