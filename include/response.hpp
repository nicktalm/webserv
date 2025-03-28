#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <map>

class Response
{
	public:
	//GET request
		std::string	getContentType();
		std::string	getStartLine(std::string protocol, std::string status_code);
		std::string getErrorMsg(std::string error);

	//POST request

	//DELETE request
};