#include "../../include/response.hpp"

std::string Response::getErrorMsg(std::string error)
{
	const std::map<std::string, std::string> errorMessages = {
		{"200", "OK"},
		{"201", "Created"},
		{"204", "No Content"},
		{"301", "Moved Permanently"},
		{"302", "Found"},
		{"304", "Not Modified"},
		{"400", "Bad Request"},
		{"401", "Unauthorized"},
		{"403", "Forbidden"},
		{"404", "Not Found"},
		{"405", "Method Not Allowed"},
		{"408", "Request Timeout"},
		{"500", "Internal Server Error"},
		{"501", "Not Implemented"},
		{"502", "Bad Gateway"},
		{"503", "Service Unavailable"},
		{"504", "Gateway Timeout"}
	};
	
	auto it = errorMessages.find(error);
	if (it != errorMessages.end())
		return(it->second);
	return("Unkown Status");
}

std::string Response::getStartLine(std::string protocol, std::string status_code)
{
	std::string start_line = protocol + " " + status_code + " " + getErrorMsg(status_code);

	return (start_line);
}

std::string Response::getContentType()
{
	return ("needs to be created (type)");
}