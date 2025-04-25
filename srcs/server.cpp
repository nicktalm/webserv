/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/24 12:22:38 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/response.hpp"
#include "../include/utils.hpp"
#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	addrinfo			hints;
	std::stringstream	tmp;
	int					yes = 1;

	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	tmp << config.port;
	if (auto status = getaddrinfo(config.server_name.c_str(), tmp.str().c_str(), &hints, &_res) != 0)
	{
		std::stringstream	error;

		error << "getaddrinfo error: " << gai_strerror(status);
		throw std::runtime_error(error.str());
	}

	_socketFd = socket(_res->ai_family, _res->ai_socktype, _res->ai_protocol);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");
	if (fcntl(_socketFd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("fcntl");
	}

	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		throw std::runtime_error("setsockopt failed");

	if (bind(_socketFd, _res->ai_addr, _res->ai_addrlen) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_socketFd, 10) < 0)
		throw std::runtime_error("listen failed");

	_clientsFd.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	close(_socketFd);
	freeaddrinfo(_res);
}

void	Server::run(void)
{
	int eventCount = poll(_clientsFd.data(), _clientsFd.size(), 0);
	if (eventCount < 0 && runner)
		throw std::runtime_error("Poll failed");
	else if (eventCount > 0)
	{
		for (size_t it = 0; it < _clientsFd.size();)
		{
			if (_clientsFd[it].revents & POLLHUP) //client disconnected or hung up
			{
				this->disconnect(_clientsFd.begin() + it);
				continue;
			}
			else if (_clientsFd[it].revents & POLLIN) //data ready to read
				this->request(_clientsFd.begin() + it);
			else if (_clientsFd[it].revents & POLLOUT) //data ready to write
			{
				int	tmpFd = _clientsFd[it].fd;
				this->response(_clientsInfo[tmpFd], _clientsFd.begin() + it);
			}
			++it;
		}
	}
}

void	Server::IO_Error(int bytesRead, std::vector<pollfd>::iterator find)
{
	if (bytesRead < 0)
		std::cerr << RED << "Recv failed" << RESET << std::endl;
	this->disconnect(find);
}

void Server::request(std::vector<pollfd>::iterator pollClient)
{
	char				tmp[8192];

	if (pollClient->fd == _socketFd) //new client trys to connect
	{
		int clientFd = accept(_socketFd, _res->ai_addr, &_res->ai_addrlen);
		if (clientFd < 0)
			std::cout << RED << "Client accept failed" << RESET << std::endl;
		else
		{
			std::cout << BLUE << "New client connected: " << clientFd << RESET << std::endl;
			fcntl(clientFd, F_SETFL, O_NONBLOCK);
			_clientsFd.push_back({clientFd, POLLIN, 0});
			_clientsInfo.emplace(std::piecewise_construct, std::forward_as_tuple(clientFd), std::forward_as_tuple()); // Client wird direkt in die map geschrieben ohen kopie zu erstellen
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(pollClient->fd, tmp, sizeof(tmp), 0);
		if (bytesRead < 0 || (bytesRead == 0 && _clientsInfo[pollClient->fd].getMsg().empty()))
			IO_Error(bytesRead, pollClient);
		else
		{
			tmp[bytesRead] = '\0';
			_clientsInfo[pollClient->fd].appendMsg(tmp, bytesRead);
			_clientsInfo[pollClient->fd].parseRequest(pollClient->fd, _config);
			if (_clientsInfo[pollClient->fd].getStatusCode() == "413")
				this->disconnect(pollClient);
			else if (!_clientsInfo[pollClient->fd].getListen() || _clientsInfo[pollClient->fd].getStatusCode()[0] == '4' || _clientsInfo[pollClient->fd].getStatusCode()[0] == '5')
				pollClient->events = POLLOUT;
		}
	}
}

std::map<std::string, std::string> parseBody(std::string body)
{
	std::map<std::string, std::string> parsedBody;
	std::istringstream stream(body);
	std::string pair;

	while (std::getline(stream, pair, '&'))
	{
		size_t pos = pair.find('=');
		if (pos  != std::string::npos)
		{
			std::string key = pair.substr(0, pos);
			std::string value = pair.substr(pos + 1);
			parsedBody[key] = value;
		}
	}
	return parsedBody;
}

// Hilfsfunktion zum Trimmen
std::string trim_server(const std::string &str)
{
	size_t first = str.find_first_not_of(" \r\n");
	size_t last = str.find_last_not_of(" \r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

// Pfad validieren – kein ../ oder /
bool isValidFilename(const std::string &filename)
{
	return filename.find("..") == std::string::npos && filename.find("/") == std::string::npos && !filename.empty();
}

void parseMultipartFormData(const std::string &body, const std::string &boundary, const std::string &uploadDir)
{
	std::string fullBoundary = "--" + boundary;
	std::string endBoundary = fullBoundary + "--";
	size_t pos = 0;
	size_t nextPart;

	while ((pos = body.find(fullBoundary, pos)) != std::string::npos)
	{
		pos += fullBoundary.length();
		if (body.compare(pos, 2, "--") == 0)
			break;
		if (body.compare(pos, 2, "\r\n") == 0)
			pos += 2;
		nextPart = body.find(fullBoundary, pos);
		if (nextPart == std::string::npos)
			break;
		std::string part = body.substr(pos, nextPart - pos);
		pos = nextPart;
		size_t headerEnd = part.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
			continue;
		std::string headers = part.substr(0, headerEnd);
		std::string content = part.substr(headerEnd + 4);
		content = trim_server(content); // Letztes \r\n entfernen
		size_t dispositionPos = headers.find("Content-Disposition:");
		if (dispositionPos == std::string::npos)
			continue;
		std::string disposition = headers.substr(dispositionPos);
		size_t namePos = disposition.find("name=\"");
		if (namePos == std::string::npos)
			continue;
		namePos += 6;
		size_t nameEnd = disposition.find("\"", namePos);
		std::string fieldName = disposition.substr(namePos, nameEnd - namePos);

		size_t filenamePos = disposition.find("filename=\"");
		if (filenamePos != std::string::npos)
		{
			filenamePos += 10;
			size_t filenameEnd = disposition.find("\"", filenamePos);
			std::string filename = disposition.substr(filenamePos, filenameEnd - filenamePos);

			if (!isValidFilename(filename))
			{
				std::cerr << "Ungültiger Dateiname: " << filename << std::endl;
				continue;
			}
			std::string filePath = uploadDir + filename;
			std::ofstream outFile(filePath, std::ios::binary);
			if (!outFile.is_open())
			{
				std::cerr << "Fehler beim Öffnen von " << filePath << std::endl;
				continue;
			}
			outFile.write(content.c_str(), content.size());
			outFile.close();
			std::cout << "✅ Datei gespeichert: " << filename << std::endl;
		}
		else
		{
			std::cout << "📄 Form-Feld: " << fieldName << " = " << content << std::endl;
		}
	}
}

std::string	trim_filending(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\n\r\f\v");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\n\r\f\v");
	return str.substr(first, last - first + 1);
}

std::string Server::handlePOST(Client &client)
{
	client.setReady(true);
	std::cout << GREEN << "POST request" << RESET << std::endl;

	if (isCGIScript(client))
		return handleCGIScript(client);

	std::string contentType = extractContentType(client.getHeader()["Content-Type"]);
	std::string uploadDir = "./http/upload/";
	std::string body = client.getBody();

	if (contentType == "multipart/form-data")
	{
		if (!handleMultipartFormData(client, body, uploadDir))
			return handleERROR(client);
	}
	else if (contentType == "application/x-www-form-urlencoded")
	{
		if (!handleURLEncoded(client, body, uploadDir))
			return handleERROR(client);
	}
	else
	{
		if (!handleRawUpload(client, body, uploadDir))
			return handleERROR(client);
	}

	return buildRedirectResponse("/websites/upload_success.html");
}

std::string Server::extractContentType(const std::string &headerValue)
{
	if (headerValue.find("multipart/form-data") != std::string::npos)
		return "multipart/form-data";
	if (headerValue.find("application/x-www-form-urlencoded") != std::string::npos)
		return "application/x-www-form-urlencoded";
	return trim(headerValue);
}

bool Server::handleMultipartFormData(Client &client, const std::string &body, const std::string &uploadDir)
{
	std::string contentTypeHeader = client.getHeader()["Content-Type"];
	size_t boundaryPos = contentTypeHeader.find("boundary=");
	if (boundaryPos == std::string::npos)
	{
		client.setStatusCode("400");
		return false;
	}

	std::string boundary = contentTypeHeader.substr(boundaryPos + 9);
	boundary.erase(std::remove(boundary.begin(), boundary.end(), '\r'), boundary.end());
	boundary.erase(std::remove(boundary.begin(), boundary.end(), '\n'), boundary.end());

	parseMultipartFormData(body, boundary, uploadDir);
	client.setStatusCode("200");
	return true;
}

bool Server::handleURLEncoded(Client &client, const std::string &body, const std::string &uploadDir)
{
	static int counter = 1;
	std::string filename = "x-www-form-urlencoded_" + std::to_string(counter++) + ".txt";
	std::ofstream outFile(uploadDir + filename);

	if (!outFile.is_open())
	{
		client.setStatusCode("500");
		return false;
	}

	outFile << "Content-Type: application/x-www-form-urlencoded\r\n";
	outFile << body;
	outFile.close();

	std::cout << GREEN << "File uploaded successfully to " << (uploadDir + filename) << RESET << std::endl;
	return true;
}

bool Server::handleRawUpload(Client &client, const std::string &body, const std::string &uploadDir)
{
	static int counter = 1;
	std::string contentTypeRaw = trim(client.getHeader()["Content-Type"]);
	std::string fileEnding;

	for (std::map<std::string, std::string>::iterator it = utils::MIMETypes.begin(); it != utils::MIMETypes.end(); ++it)
	{
		if (it->first == contentTypeRaw)
		{
			fileEnding = it->second;
			break;
		}
	}

	if (fileEnding.empty())
	{
		std::cout << RED << "WARNUNG: Unbekannter Content-Type, Dateiendung kann nicht ermittelt werden." << RESET << std::endl;
		client.setStatusCode("415");
		return false;
	}

	std::string filename = "uploaded_file_" + std::to_string(counter++) + fileEnding;
	std::ofstream outFile(uploadDir + filename);
	if (!outFile.is_open())
	{
		client.setStatusCode("500");
		return false;
	}

	outFile << "Content-Type: " << contentTypeRaw << "\r\n";
	outFile << body;
	outFile.close();

	std::cout << GREEN << "File uploaded successfully to " << (uploadDir + filename) << RESET << std::endl;
	return true;
}

std::string Server::buildRedirectResponse(const std::string &location)
{
	std::string response = "HTTP/1.1 303 See Other\r\n";
	response += "Location: " + location + "\r\n";
	response += "Content-Length: 0\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	return response;
}


void	Server::response(Client &client, std::vector<pollfd>::iterator pollClient)
{
	std::cout << BLUE << "Response" << RESET << std::endl;
	std::cout << "client = " << client.getFd() << std::endl;
	if (client.getBytesSend() == static_cast<ssize_t>(client.getResponseBuffer().size()))
	{
		if (client.getStatusCode()[0] == '4' || client.getStatusCode()[0] == '5')
			client.setResponseBuffer(handleERROR(client));
		else if (client.getMethod() == "GET")
			client.setResponseBuffer(handleGET(client));
		else if(client.getMethod() == "POST")
			client.setResponseBuffer(handlePOST(client));
		else if(client.getMethod() == "DELETE")
			client.setResponseBuffer(handleDELETE(client));
	}

	std::string	response = client.getResponseBuffer();
	ssize_t		bytesSent = client.getBytesSend();
	ssize_t		remaining = response.size() - bytesSent;
	ssize_t		bytes = 0;

	// Send as much as possible
	bytes = send(client.getFd(), response.c_str() + bytesSent, remaining, 0);
	if (bytes <= 0)
	{
		std::cout << RED << "send failed" << RESET << std::endl;
		this->disconnect(pollClient);
		return;
	}
	ssize_t	currBytes = bytesSent + bytes;
	client.setBytesSend(currBytes);
	// nachdem alles gesendet wurde, client aud POLLIN einstellen
	if (currBytes == static_cast<ssize_t>(response.size()))
	{
		client.setResponseBuffer(""); // Clear buffer fuer naechstes mal
		client.setBytesSend(0);
	}
	if (client.getReady() && (currBytes == static_cast<ssize_t>(response.size())))
	{
		pollClient->events = POLLIN;
		client.clear();
	}
}

std::string	Server::handleERROR(Client &client)
{
	t_response	response;
	std::string	errorMsg;
	std::string	path;
	size_t		end;

	errorMsg = client.getErrorMsg(client.getStatusCode());
	end = errorMsg.find(':');
	if (!client.getLocationInfo().error_page.empty() && client.getLocationInfo().error_page.find(client.getStatusCode()) != client.getLocationInfo().error_page.end())
	{
		path = client.getLocationInfo().error_page.find(client.getStatusCode())->second;
		std::cout << "path = " << path << std::endl;
	}
	else
		path = errorMsg.substr(end + 2);
	if (!utils::readFile(path, response.body))
		return (client.setStatusCode("404"), handleERROR(client));
	response.start_line = "HTTP/1.1 " + client.getStatusCode() + " " + errorMsg.substr(0, end);
	response.server_name = "Servername: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + client.getContentType(path);
	client.setReady(true);
	return (Server::create_response(response));
}

std::string Server::handleDELETE(Client &client)
{
	std::cout << PURPLE << "DELETE method" << RESET << std::endl;
	std::string path = client.getPath();
	if (std::remove(path.c_str()) != 0)
	{
		std::cout << RED << "File doesn't exist" << RESET << std::endl;
		return (handleERROR(client));
	}
	else
		std::cout << GREEN << "DELETE successful" << RESET << std::endl;
	client.setReady(true);
	return "HTTP/1.1 204 No Content\r\n\r\n";
}

// TODO man muss noch check ob die error page vorhanden ist in der config file

std::string Server::create_response(const t_response &response)
{
	std::string finished;

	finished =
	response.start_line + "\r\n" +
	response.server_name + "\r\n" +
	response.date + "\r\n" +
	response.content_length + "\r\n" +
	response.content_type + "\r\n" +
	response.empty_line +
	response.body  + "\r\n";
	return (finished);
}

std::string	Server::handleGET(Client &client)
{
	std::cout << PURPLE << "GetRequest" << RESET << std::endl;
	std::string	body;

	if (!client.getHeaderReady())
	{
		std::stringstream	tmpHeader;

		tmpHeader << client.getProtocol() << " " + client.getStatusCode() << " " << client.getErrorMsg(client.getStatusCode()) << "\r\n";
		tmpHeader << "Server: " << this->_config.server_name << "\r\n";
		tmpHeader << "Date: " << utils::getDate() << "\r\n";

		if (client.getLocationInfo().autoindex && client.getPath().back() == '/')
			tmpHeader << "Content-Type: text/html\r\n" << "Transfer-Encoding: chunked\r\n";
		else if (!client.getReDir().empty())
		{
			tmpHeader << client.getReDir() << "\r\n";
			client.setReady(true);
		}
		else
		{
			std::string	body;

			if (!utils::readFile(client.getPath(), body))
				return (client.setStatusCode("404"), handleERROR(client));
			tmpHeader << "Content-Type: " << client.getContentType(client.getPath()) << "\r\n";
			tmpHeader << "Content-Length: " << body.size() << "\r\n";
		}
		tmpHeader << "\r\n";
		client.setHeaderReady(true);
		return (tmpHeader.str());
	}
	if (client.getLocationInfo().autoindex && client.getPath().back() == '/')
	{
		if (client.getAutoIndexPart() == 0)
		{
			std::string			tmp;
			size_t				pos;
			std::stringstream	size;

			tmp = utils::autoindexHead;
			while ((pos = tmp.find("{{path}}")) != std::string::npos)
				tmp.replace(pos, 8, client.getPath());
			size << std::hex << tmp.size();
			tmp.insert(0, size.str() + "\r\n");
			tmp.append("\r\n");
			client.setAutoIndexPart(1);
			return (tmp);
		}
		else if (client.getAutoIndexPart() == 1)
		{
			dirent				*entry;
			std::string			repo;
			std::stringstream	size;

			entry = readdir(client.getDir());
			if (entry != nullptr && std::string(entry->d_name) == ".")
				entry = readdir(client.getDir());
			if (entry != nullptr)
			{
				repo = client.createAutoIndex(client.getPath(), entry->d_name);
				size << std::hex << repo.size();
				repo.insert(0, size.str() + "\r\n");
				repo.append("\r\n");
				return (repo);
			}
			client.setAutoIndexPart(2);
		}
		if (client.getAutoIndexPart() == 2)
		{
			std::string	tmp = utils::autoindexBody;
			std::stringstream	size;

			size << std::hex << tmp.size();
			tmp.insert(0, size.str() + "\r\n");
			tmp.append("\r\n");
			client.setAutoIndexPart(3);
			return (tmp);
		}
		if (client.getAutoIndexPart() == 3)
		{
			client.setAutoIndexPart(0);
			client.setReady(true);
			return ("0\r\n\r\n");
		}
	}
	else
	{
		client.setReady(true);
		if (!utils::readFile(client.getPath(), body))
			return (client.setStatusCode("404"), handleERROR(client));
	}
	return (body);
}

void	Server::disconnect(std::vector<pollfd>::iterator find)
{
	std::cout << YELLOW << "Client disconnected: " << find->fd << RESET << std::endl;
	auto del = _clientsInfo.find(find->fd);
	_clientsInfo.erase(del);
	close(find->fd);
	_clientsFd.erase(find);
}
