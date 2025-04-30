#pragma once

#include <string>
#include <map>
#include <vector>
#include <dirent.h>

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
		bool						_exeCGI;
		bool						_responseHeader;
		bool						_responseReady;
		bool						_waitForChild;
		bool						_firstTime;
		int							_autoIndexPart;
		int							_CGIOutput;
		ssize_t						_bytesSend;
		pid_t						_childId;
		std::string					_responseBuffer;
		std::string					_reDirHeader;
		DIR							*_dir;
	public:
		Response(void);
		virtual ~Response(void);

		// getter
		bool						getHeaderReady(void) const {return (_responseHeader);};
		bool						getReady(void) const {return (_responseReady);};
		bool						getCGI(void) const {return (_exeCGI);};
		bool						getChildReady(void) const {return (_waitForChild);};
		bool						getFirstTime(void) const {return (_firstTime);};
		int							getAutoIndexPart(void) const {return (_autoIndexPart);};
		int							getCGIOutput(void) const {return (_CGIOutput);};
		ssize_t						getBytesSend(void) const { return (_bytesSend);};
		pid_t						getChildId(void) const {return (_childId);};
		std::string					getResponseBuffer(void) const {return (_responseBuffer);};
		DIR							*getDir(void) const {return (_dir);};

		// setter

		void		setHeaderReady(const bool header) {_responseHeader = header;};
		void		setResponseBuffer(const std::string &response) {_responseBuffer = response;};
		void		setBytesSend(const ssize_t bytes) {_bytesSend = bytes;};
		void		setReady(const bool ready) {_responseReady = ready;};
		void		setAutoIndexPart(const int part) {_autoIndexPart = part;};
		void		setChildReady(const bool ready) {_waitForChild = ready;};
		void		setChildId(const pid_t id) {_childId = id;};
		void		setCGIOutput(const int pipe) {_CGIOutput = pipe;};
		void		setFirstTime(const bool check) {_firstTime = check;};
	//GET request
		std::string	getContentType(std::string file);
		std::string	getStartLine(std::string protocol, std::string status_code);
		std::string getErrorMsg(std::string error);

	//POST request

	//DELETE request
};