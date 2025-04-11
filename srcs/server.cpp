/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/11 12:23:47 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
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
	fcntl(_socketFd, F_SETFL, O_NONBLOCK);

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
	char				tmp[1024];

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
			std::cout << "bytesRead = " << bytesRead << std::endl;
			tmp[bytesRead] = '\0';
			_clientsInfo[pollClient->fd].getTestMsg() << tmp;
			// _clientsInfo[pollClient->fd].appendMsg(tmp, bytesRead);
			_clientsInfo[pollClient->fd].parseRequest(pollClient->fd, _config, _clientsInfo[pollClient->fd].getTestMsg());
			if (!_clientsInfo[pollClient->fd].getListen())
				pollClient->events = POLLOUT;
			if (_clientsInfo[pollClient->fd].getstatusCode()[0] != '2' && _clientsInfo[pollClient->fd].getstatusCode()[0] != '3')
				this->response(_clientsInfo[pollClient->fd], pollClient);
		}
	}
}

std::map<std::string, std::string> parseBody(std::string &body)
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

std::string execute_cgi(Client &client, std::string path)
{
	int inputPipe[2], outputPipe[2];
	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
	{
		std::cerr << RED << "Pipe creation failed" << RESET << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);
		std::cerr << RED << "Fork failed" << RESET << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}
	else if (pid == 0) // Child process
	{
		dup2(inputPipe[0], STDIN_FILENO);
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		dup2(outputPipe[1], STDOUT_FILENO);
		close(outputPipe[1]);
		std::map<std::string, std::string> params = parseBody(client.getBody());

		char *const args[] = {
			const_cast<char *>(path.c_str()),
			const_cast<char *>(params["username"].c_str()), 
			const_cast<char *>(params["password"].c_str()),
			nullptr};
		if (execve(path.c_str(), args, nullptr) == -1)
		{
			std::cerr << RED << "Execve failed" << RESET << std::endl;
			return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
		}
	}
	else // Parent process
	{
		close(inputPipe[0]);
		write(inputPipe[1], client.getBody().c_str(), client.getBody().size());
		close(inputPipe[1]);

		close(outputPipe[1]);
		char buffer[1024];
		std::string cgiOutput;
		ssize_t bytesRead;
		while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
		{
			cgiOutput.append(buffer, bytesRead);
		}
		close(outputPipe[0]);

		int status;
		waitpid(pid, &status, 0);

		return cgiOutput;
	}
	return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
}

std::string Server::handlePOST(Client &client)
{
	std::cout << GREEN << "POST request" << RESET << std::endl;
	std::cout << client.getPath() << std::endl;
	if (client.getPath() == "http/cgi-bin/register.py")
	{
		return (execute_cgi(client, "./http/cgi-bin/register.py"));
	}
	if (client.getPath() == "http/cgi-bin/signup.py")
	{
		return (execute_cgi(client, "./http/cgi-bin/signup.py"));
	}
	// std::cout << "Body: " << client.getBody() << std::endl;
	std::string uploadDir = "./http/upload/";
	std::string body = client.getBody();

	// Extract filename from the body
	size_t filenamePos = body.find("filename=\"");
	if (filenamePos == std::string::npos)
	{
		std::cerr << RED << "Filename not found in request body" << RESET << std::endl;
		return "HTTP/1.1 400 Bad Request\r\n\r\n";
	}
	filenamePos += 10; // Move past 'filename="'
	size_t filenameEnd = body.find("\"", filenamePos);
	if (filenameEnd == std::string::npos)
	{
		std::cerr << RED << "Invalid filename format" << RESET << std::endl;
		return "HTTP/1.1 400 Bad Request\r\n\r\n";
	}
	std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);

	// Extract file content from the body
	size_t contentStart = body.find("\r\n\r\n", filenameEnd);
	if (contentStart == std::string::npos)
	{
		std::cerr << RED << "File content not found in request body" << RESET << std::endl;
		return "HTTP/1.1 400 Bad Request\r\n\r\n";
	}
	contentStart += 4; // Move past the "\r\n\r\n"
	size_t contentEnd = body.find("------WebKitFormBoundary", contentStart);
	if (contentEnd == std::string::npos)
	{
		std::cerr << RED << "Invalid file content format" << RESET << std::endl;
		return "HTTP/1.1 400 Bad Request\r\n\r\n";
	}
	std::string fileContent = body.substr(contentStart, contentEnd - contentStart);

	// Write content to the file
	std::string filePath = uploadDir + filename;
	std::ofstream outFile(filePath);
	if (outFile.is_open())
	{
		outFile << fileContent;
		outFile.close();
		std::cout << GREEN << "File uploaded successfully to " << filePath << RESET << std::endl;
	}
	else
	{
		std::cerr << RED << "Error opening file for writing" << RESET << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}

	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/plain\r\n";
	response += "Content-Length: 0\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += "File uploaded successfully\r\n";
	
	return response;
}


void	Server::response(Client &client, std::vector<pollfd>::iterator pollClient)
{
	std::cout << BLUE << "Response" << RESET << std::endl;
	if (client.getResponseBuffer().empty())
	{
		std::string response;
		if (client.getstatusCode()[0] != '2' && client.getstatusCode()[0] != '3')
			response = handleERROR(client);
		else if (client.getMethod() == "GET")
			response = handleGET(client);
		else if(client.getMethod() == "POST")
			response = handlePOST(client);
		else if(client.getMethod() == "DELETE")
			response = handleDELETE(client);
		// else
		// {
			// response = "";
		// }
		client.setResponseBuffer(response);
		client.getBytesSent() = 0;
	}

	std::string response = client.getResponseBuffer();
	size_t bytesSent = client.getBytesSent();
	ssize_t remaining = response.size() - bytesSent;

	// Send as much as possible
	ssize_t sent = send(client.getFd(), response.c_str() + bytesSent, remaining, 0);
	if (sent <= 0)
	{
		std::cout << RED << "send failed" << RESET << std::endl;
		close(client.getFd());
		return;
	}
	bytesSent += sent;
	// nachdem alles gesendet wurde, client aud POLLIN einstellen
	if (bytesSent >= response.size())
	{
		pollClient->events = POLLIN;
		client.getResponseBuffer().clear(); // Clear buffer fuer naechstes mal
	}
}

std::string	Server::handleERROR(Client &client)
{
	Response	repo;
	t_response	response;
	std::string	errorMsg;
	std::string	path;
	size_t		end;

	std::cout << "status code = " << client.getstatusCode() << std::endl;
	errorMsg = repo.getErrorMsg(client.getstatusCode());
	end = errorMsg.find(':');
	path = errorMsg.substr(end + 2);
	if (utils::readFile(path, response.body) == false)
		return (client.setStatusCode("404"), handleERROR(client));
	response.start_line = "HTTP/1.1 " + client.getstatusCode() + " " + errorMsg.substr(0, end);
	response.server_name = "Servername: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + repo.getContentType(path);
	
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
	Response responseInstance;
	t_response response;
	std::string	path;
	
	path = client.getPath();
	if (path.empty())
		return (handleERROR(client));
	else
	{
		if (!client.getReDir().empty())
		{
			std::stringstream	tmp;
			response.server_name = "Servername: " + this->_config.server_name + "\r\n";
			response.date = "Date: " + utils::getDate() + "\r\n";
			response.start_line = responseInstance.getStartLine(client.getProtocol(), client.getstatusCode()) + "\r\n";
			response.content_type = client.getReDir() + "\r\n";

			tmp << response.start_line << response.server_name << response.date << response.content_type << response.empty_line;
			return (tmp.str());
		}
		else if (!client.getAutoIndex().empty())
			response.body = client.getAutoIndex();
		else
		{
			if (!utils::readFile(path, response.body))
				return (client.setStatusCode("404"), handleERROR(client));
		}
	}
	response.server_name = "Servername: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + responseInstance.getContentType(path);
	response.start_line = responseInstance.getStartLine(client.getProtocol(), client.getstatusCode());
	return (Server::create_response(response));
}

void	Server::disconnect(std::vector<pollfd>::iterator find)
{
	std::cout << YELLOW << "Client disconnected: " << find->fd << RESET << std::endl;
	auto del = _clientsInfo.find(find->fd);
	_clientsInfo.erase(del);
	close(find->fd);
	_clientsFd.erase(find);
}
