#pragma once

#include <string>
#include <map>

typedef struct s_response
{
	std::string	start_line = "";
	std::string	date = "";
	std::string	content_type = "";
	std::string	content_length = "";
	std::string empty_line = "\r\n";
	std::string	body = "";
	std::string server_name = "";
}	t_response;

class Response
{
	public:
	//GET request
		std::string	getContentType(std::string file);
		std::string	getStartLine(std::string protocol, std::string status_code);
		std::string getErrorMsg(std::string error);

	//POST request

	//DELETE request
};