#include "../../include/response.hpp"
#include "../../include/utils.hpp"
#include <iostream>

Response::Response(void)
{
	_client = client;
	_responseBuffer = "";
	_responseReady = false;
	_bytesSend = 0;
}

Response::~Response(void) {}

std::string Response::getErrorMsg(std::string error)
{
	const std::map<std::string, std::string> errorMessages = {
		{"200", "OK"},
		{"201", "Created"},
		{"204", "No Content"},
		{"301", "Moved Permanently"},
		{"302", "Found"},
		{"304", "Not Modified"},
		{"400", "Bad Request: error_pages/400.html"},
		{"401", "Unauthorized"},
		{"403", "Forbidden: error_pages/403.html"},
		{"404", "Not Found: error_pages/404.html"},
		{"405", "Method Not Allowed: error_pages/405.html"},
		{"408", "Request Timeout"},
		{"413", "Payload Too Large: error_pages/413.html"},
		{"431", "Request Header Fields Too Large: error_pages/431.html"},
		{"500", "Internal Server Error"},
		{"501", "Not Implemented"},
		{"502", "Bad Gateway"},
		{"503", "Service Unavailable"},
		{"504", "Gateway Timeout"},
		{"505", "HTTP Version Not Supported: error_pages/505.html"}
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

std::string Response::getContentType(std::string file)
{
	std::string	extention, ret;
	size_t	end;

	if (file == "autoindex")
		return (utils::MIMETypes[".html"]);
	end = file.find('.');
	if (end != std::string::npos)
	{
		extention = file.substr(end);
		auto it = utils::MIMETypes.find(extention);
		if (it != utils::MIMETypes.end())
			return (utils::MIMETypes[extention]);
	}
	return ("");
}