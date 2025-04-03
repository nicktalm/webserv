#pragma once

#include <string>
#include <vector>
#include <map>
#include "server.hpp"

namespace utils
{
	// parsing returns the servers created by the config file
	std::vector<std::unique_ptr<Server>>	parsing(int argc, char **argv);

	// reads file and return the input as an std::string
	bool			readFile(std::string input, std::string &body);

	// returns the current date
	std::string	getDate(void);

	// parsing the MIME file
	void	parseMIME(void);

	// maps all the MIME Types
	extern std::map<std::string, std::string>	MIMETypes;
}
