/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/04/28 19:57:33 by lbohm            ###   ########.fr       */
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

std::string	Server::execute_cgi(Client &client)
{
	int		pipeIn[2];
	int		pipeOut[2];
	pid_t	pid;

	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
	{
		std::cerr << RED; std::cerr << perror("pipe"); << std::cerr << RESET;
		client.setStatusCode("500");
		return (handleERROR(client));
	}

	pid = fork();
	if (pid == -1)
	{
		std::cerr << RED; perror("fork"); << std::cerr << RESET;
		if (close(pipeIn[0]) == -1 || close(pipeIn[1]) == -1
			|| close(pipeOut[0] == -1 || close(pipeOut[1] == -1)))
			std::cerr << RED; perror("close"); std::cerr << RESET;
		client.setStatusCode("500");
		return (handleERROR(client));
	}
	
	if (pid == 0)
		this->childProcess(client);
	

}

void	Server::childProcess(Client &client)
{
	std::vector<std::string> envpStrings;
	std::vector<char*> envp;

	this->createEnv(client, envpStrings, envp);
	size_t	pos = client.getExePath().rfind('/');
	std::string	exe = client.getExePath().substr(pos + 1);
	std::string	scriptPath = client.getPath();
	char *const args[] = {const_cast<char *>(exe.c_str()), const_cast<char *>(scriptPath.c_str()), nullptr};

	if (dup2(pipeIn[0], STDIN_FILENO) == -1 || dup2(pipeOut[1], STDOUT_FILENO) == -1)
	{
		std::cerr << RED; perror("dup2"); std::cerr << RESET;
		exit (500);
	}
	
	if (close(pipeIn[1]) == -1 || close(pipeOut[0]) == -1)
	{
		std::cerr << RED; perror("close"); std::cerr << RESET;
		exit (500);
	}

	if (execve(client.getExePath().c_str(), args, envp.data()) == -1)
	{
		std::cerr << RED; perror("execve"); std::cerr << RESET;
		exit (500);
	}
}

void	Server::parentProcess(Client &client, pid_t pid)
{
	int		status;
	int		childPid;

	if (dup2(pipeIn[1], STDOUT_FILENO) == -1 || dup2(pipeOut[0], STDIN_FILENO) == -1)
	{
		std::cerr << RED; perror("dup2"); std::cerr << RESET;
		client.setStatusCode("500");
		return (handleERROR(client));
	}

	if (close(pipeIn[0]) == -1 || close(pipeOut[1]) == -1)
	{
		std::cerr << RED; perror("close"); std::cerr << RESET;
		client.setStatusCode("500");
		return (handleERROR(client));
	}

	if (write(pipeIn[1], client.getBody().c_str(), client.getBody().size()) == -1)
	{
		std::cerr << RED; perror("write"); std::cerr << RESET;
		client.setStatusCode("500");
		return (handleERROR(client));
	}

	childPid = waitpid(pid, &status, WNOHANG);
	if (childPid == 0)
		client.setChildReady(true);
	else if (childPid == -1)
	{
		std::cerr << RED; perror("waitpid"); std::cerr << RESET;
		client.setChildReady(false);
		client.setStatusCode("500");
		return (handleERROR(client));
	}
	else
	{
		
		client.setChildReady(false);
		break ;
	}
}

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
		std::cout << "output" << std::endl;
		std::cout << cgiOutput << std::endl;
		return (cgiOutput);
	}
	return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
}

void	Server::createEnv(Client &client, std::vector<std::string> &envpStrings, std::vector<char *> &envs)
{
	envpStrings.push_back("AUTH_TYPE=null");
	envpStrings.push_back("CONTENT_LENGTH=" + client.getHeader()["Content-Length"]);
	envpStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envpStrings.push_back("PATH_INFO=" + client.getPathInfo());
	envpStrings.push_back("PATH_TRANSLATED=" + client.getPath());
	envpStrings.push_back("QUERY_STRING=" + client.getQuery());
	envpStrings.push_back("SCRIPT_NAME=" + client.getPath());
	envpStrings.push_back("SERVER_NAME=" + _config.server_name);
	envpStrings.push_back("SERVER_PORT=" + std::to_string(_config.port));
	envpStrings.push_back("SERVER_PROTOCOL=" + client.getProtocol());
	envpStrings.push_back("HTTP_COOKIE=" + client.getHeader()["Cookie"]);
	envpStrings.push_back("REQUEST_METHOD=" + client.getMethod());
	envpStrings.push_back("CONTENT_TYPE=" + client.getHeader()["Content-Type"]);
	
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