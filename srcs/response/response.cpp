#include "../../include/response.hpp"
#include "../../include/utils.hpp"
#include <iostream>

Response::Response(void)
{
	_exeCGI = false;
	_responseHeader = false;
	_responseReady = false;
	_waitForChild = false;
	_firstTime = false;
	_autoIndexPart = 0;
	_CGIOutput = 0;
	_bytesSend = 0;
	_childId = 0;
	_responseBuffer = "";
	_reDirHeader = "";
	_dir = nullptr;
}

Response::~Response(void)
{
	if (_dir != nullptr)
	{
		if (closedir(_dir) != -1)
			_dir = nullptr;
	}
}

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
		{"401", "Unauthorized: error_pages/504.html"},
		{"403", "Forbidden: error_pages/403.html"},
		{"404", "Not Found: error_pages/404.html"},
		{"405", "Method Not Allowed: error_pages/405.html"},
		{"408", "Request Timeout: error_pages/408.html"},
		{"413", "Payload Too Large: error_pages/413.html"},
		{"431", "Request Header Fields Too Large: error_pages/431.html"},
		{"500", "Internal Server Error: error_pages/500.html"},
		{"501", "Not Implemented: error_pages/501.html"},
		{"502", "Bad Gateway: error_pages/502.html"},
		{"503", "Service Unavailable: error_pages/503.html"},
		{"504", "Gateway Timeout: error_pages/504.html"},
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