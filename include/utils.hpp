#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <sstream>

class Server;

namespace utils
{
	// parsing returns the servers created by the config file
	std::vector<Server>	parsing(int argc, char **argv);

	// reads file and return the input as an std::string
	std::string	readFile(std::string input);

	// returns the current date
	std::string	getDate(void);

	// parsing the MIME file
	void	parseMIME(void);

	// maps all the MIME Types
	extern std::map<std::string, std::string>	MIMETypes;
}
