#include "../include/server.hpp"
#include "../include/utils.hpp"

std::atomic<bool>	run = true;

int main(int argc, char **argv)
{
	try
	{
		std::vector<Server>	servers;

		servers = utils::parsing(argc, argv);
		while(run)
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