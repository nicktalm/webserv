#include "../include/server.hpp"

std::atomic<bool>	run = true;

bool check_config(std::string config_path, std::vector<t_config> &files)
{
	(void)config_path;
	t_config	tmp;

	tmp.port = 8080;
	tmp.server_name = "localhost";

	files.push_back(tmp);
	return (true);
}

int main(int argc, char **argv)
{
	std::vector<t_config>	files;

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
		if (!check_config(config_file, files))
		{
			std::cout << RED << "Error in config file" << RESET << std::endl;
			return EXIT_FAILURE;
		}
	}
	
	try
	{
		std::vector<Server> servers;

		for (auto config : files)
			servers.push_back(Server(config));
		while(run)
		{
			for (auto server : servers)
			{
				server.run();
			}
		}

	}
	catch (std::exception &e)
	{
		std::cout << RED << e.what() << RESET << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

//TODO: implement signal handler that puts run bool to false
//TODO: implement better structure for the run function