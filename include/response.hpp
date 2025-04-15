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
		int							_currentFile;
	public:
		Response(void);
		virtual ~Response(void);

		// getter
		std::string					getResponseBuffer(void) const {return _responseBuffer;};
		ssize_t						getBytesSend(void) const { return _bytesSend;};
		bool						getReady(void) const {return (_responseReady);};
		bool						getAutoIndex(void) const {return (_autoIndex);};
		int							getAutoIndexPart(void) const {return (_autoIndexPart);};
		std::vector<std::string>	getFiles(void) const {return (_files);};
		int							getCurrentFileIndex(void) const {return (_currentFile);};

		// setter

		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		void		setBytesSend(const ssize_t bytes) {_bytesSend = bytes;};
		void		setReady(const bool ready) {_responseReady = ready;};
		void		setAutoIndexPart(const int part) {_autoIndexPart = part;};
		void		setCurrentFileIndex(const int index) {_currentFile = index;};
	//GET request
		std::string	getContentType(std::string file);
		std::string	getStartLine(std::string protocol, std::string status_code);
		std::string getErrorMsg(std::string error);

	//POST request

	//DELETE request
};