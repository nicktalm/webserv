/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/04/28 13:02:03 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "../include/server.hpp"

std::string decodeURIcomponent(std::string &shoppinglist)
{
	std::string trimmedList;
	for (unsigned long i = 0; i < shoppinglist.size(); i++)
	{
		if (shoppinglist[i] == '%' &&
			shoppinglist[i + 1] == '2' &&
			shoppinglist[i + 2] == 'C')
			{
				trimmedList += ',';
				i += 2;
			}
		else if (shoppinglist[i] == '+')
			trimmedList += " ";
		else
			trimmedList += shoppinglist[i];
	}
	return trimmedList;
}

//TODO fix blocking
std::string Server::execute_cgi(Client &client)
{
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		std::cerr << RED << "Pipe creation failed" << RESET << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}
		
	pid_t pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		std::cerr << RED; perror("fork"); std::cerr << RESET;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}	
	else if (pid == 0) // Child process
	{
		std::vector<std::string> envpStrings;
		std::vector<char*> envp;

		this->createEnv(client, envpStrings, envp);
		
		size_t	pos = client.getExePath().rfind('/');
		std::string	exe = client.getExePath().substr(pos + 1);
		std::string	scriptPath = client.getPath();
		char *const args[] = {const_cast<char *>(exe.c_str()), const_cast<char *>(scriptPath.c_str()), nullptr};
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		if (execve(client.getExePath().c_str(), args, envp.data()) == -1)
		{
			std::cerr << RED << "Execve failed" << RESET << std::endl;
			return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
		}
	}
	else // Parent process
	{
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[1]);
		char buffer[1024];
		std::string cgiOutput;
		ssize_t bytesRead;
		while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0)
		{
			cgiOutput.append(buffer, bytesRead);
		}
		close(pipefd[0]);
		int status;
		waitpid(pid, &status, 0);
		return (cgiOutput);
	}
	return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
}

void	Server::createEnv(Client &client, std::vector<std::string> &envpStrings, std::vector<char *> &envs)
{
	std::string cookie = client.getHeader()["Cookie"];
	// SCRIPT_NAME	Pfad zum Skript (relativ zum Root)
	// PATH_INFO	Extra-Path-Info nach dem Skriptnamen
	envpStrings.push_back("REQUEST_METHOD=" + client.getMethod());
	envpStrings.push_back("CONTENT_TYPE=" + client.getHeader()["Content-Type"]);
	envpStrings.push_back("CONTENT_LENGTH=" + client.getHeader()["Content-Length"]);
	envpStrings.push_back("SERVER_NAME=" + _config.server_name);
	envpStrings.push_back("SERVER_PORT=" + std::to_string(_config.port));
	envpStrings.push_back("SERVER_PROTOCOL=" + client.getProtocol());
	envpStrings.push_back("PATH_TRANSLATED=" + client.getPath());
	envpStrings.push_back("HTTP_COOKIE=" + cookie);
	envpStrings.push_back("QUERY_STRING=" + client.getQuery());

	for(size_t i = 0; i < envpStrings.size(); i++)
	{
		envs.push_back(const_cast<char*>(envpStrings[i].c_str()));
	}
	envs.push_back(nullptr);
}

std::string Server::handleCGIScript(Client &client)
{
	std::string path = client.getPath();
	std::string cgi_type = "";
	size_t pos = path.rfind('.');
	if (pos != std::string::npos)
		cgi_type = path.substr(pos);

	std::cout << "Type: " << cgi_type << std::endl;

	if (cgi_type == ".py") //hier mehr types adden wie .php etc.
	{
		std::string final_path = "./" + path;
		return execute_cgi(client);
	}
	return "";
}


bool Server::isCGIScript(Client &client)
{
	std::string path = client.getPath();
	std::string cgi_type = "";
	size_t pos = path.rfind('.');
	if (pos != std::string::npos)
		cgi_type = path.substr(pos);
	if(cgi_type == ".py")//hier auch wieder mehrer types adden wie nohc .php etc.
		return true;
	return false;
}