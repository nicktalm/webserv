#pragma once

#include <string>
#include <map>
#include <vector>

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
	protected:
		bool						_responseReady;
		bool						_autoIndex;
		int							_autoIndexPart;
		std::string					_responseBuffer;
		ssize_t						_bytesSend;
		std::vector<std::string>	_files;
	public:
		Response(void);
		virtual ~Response(void);

		// getter
		std::string	getResponseBuffer(void) {return _responseBuffer;} const;
		ssize_t		getBytesSend(void) { return _bytesSend;} const;
		bool		getReady(void) {return (_responseReady);} const;
		bool		getAutoIndex(void) {return (_autoIndex);} const;
		int			getAutoIndexPart(void) {return (_autoIndexPart);} const;

		// setter

		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		void		setBytesSend(const size_t bytes) {_bytesSend = bytes;};
		void		setReady(const bool ready) {_responseReady = ready;};
	//GET request
		std::string	getContentType(std::string file);
		std::string	getStartLine(std::string protocol, std::string status_code);
		std::string getErrorMsg(std::string error);

	//POST request

	//DELETE request
};