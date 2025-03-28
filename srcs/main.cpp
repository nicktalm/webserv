#include "../include/server.hpp"

void	signalHandler(int signal)
{
	std::cout << YELLOW << "\rInterrupt signal " << signal << " received" << RESET << std::endl;
	test = false;
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler); //ctrl-c
	try
	{
		std::vector<Server>	servers;

		servers = utils::parsing(argc, argv);
		while(test)
		{
			for (auto it = servers.begin(); it != servers.end(); ++it)
			{
				it->run();
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