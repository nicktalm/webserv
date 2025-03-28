#pragma once

#include <iostream>
#include <vector>

class Server;

namespace utils
{
	// parsing returns the servers created by the config file
	std::vector<Server>	parsing(int argc, char **argv);

	// reads file and return the input as an std::string
	std::string	readFile(std::string input);
	std::string getDate(void);
}
