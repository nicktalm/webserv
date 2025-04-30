/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/04/30 15:06:35 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/server.hpp"

std::string	Server::checkCGI(Client &client)
{
	bool	check = true;

	if (!client.getFirstTime())
	{
		if (!client.getChildReady())
			check = this->execute_cgi(client);
		else
			check = this->waitingroom(client);
		if (!client.getChildReady())
			client.setFirstTime(true);
	}
	if (check && !client.getChildReady())
	{
		std::string	tmp;

		tmp = this->readFromFd(client);
		if (client.getReady())
			client.setFirstTime(false);
		return (tmp);
	}
	return ("");
}

bool	Server::execute_cgi(Client &client)
{
	int		pipeIn[2];
	int		pipeOut[2];

	std::cout << RED << "Start CGI" << RESET << std::endl;
	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
	{
		std::cerr << RED; perror("pipe"); std::cerr << RESET;
		client.setStatusCode("500");
		return (false);
	}

	client.setChildId(fork());
	if (client.getChildId() == -1)
	{
		std::cerr << RED; perror("fork"); std::cerr << RESET;
		if (close(pipeIn[0]) == -1 || close(pipeIn[1]) == -1
			|| close(pipeOut[0]) == -1 || close(pipeOut[1]) == -1)
			std::cerr << RED; perror("close"); std::cerr << RESET;
		client.setStatusCode("500");
		return (false);
	}
	
	if (client.getChildId() == 0)
		this->childProcess(client, pipeIn, pipeOut);

	if (!this->parentProcess(client, pipeIn, pipeOut))
		return (false);
	return (true);
}

void	Server::childProcess(Client &client, int *pipeIn, int *pipeOut)
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
	
	if (close(pipeIn[1]) == -1 || close(pipeOut[0]) == -1
		|| close(pipeIn[0]) == -1 || close(pipeOut[1]) == -1)
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

bool	Server::parentProcess(Client &client, int *pipeIn, int *pipeOut)
{
	if (write(pipeIn[1], client.getBody().c_str(), client.getBody().size()) == -1)
	{
		std::cerr << RED << "Write failed" << RESET << std::endl;
		client.setStatusCode("500");
		return (false);
	}

	if (close(pipeIn[0]) == -1 || close(pipeOut[1]) == -1
		|| close(pipeIn[1]) == -1)
	{
		std::cerr << RED; perror("close"); std::cerr << RESET;
		client.setStatusCode("500");
		return (false);
	}

	if (fcntl(pipeOut[0], F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << RED; perror("fcntl"); std::cerr << RESET;
		return (false);
	}
	client.setCGIOutput(pipeOut[0]);
	return (this->waitingroom(client));
}

bool	Server::waitingroom(Client &client)
{
	int		status;
	int		childPid;

	childPid = waitpid(client.getChildId(), &status, WNOHANG);
	if (childPid == 0)
	{
		client.setChildReady(true);
		return (true);
	}
	else if (childPid == -1)
	{
		std::cerr << RED; perror("waitpid"); std::cerr << RESET;
		client.setChildReady(false);
		client.setStatusCode("500");
		return (false);
	}
	else
	{
		client.setChildReady(false);

		if (WEXITSTATUS(status))
		{
			client.setStatusCode(std::to_string(WEXITSTATUS(status)));
			return (false);
		}
	}
	return (true);
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
		envs.push_back(const_cast<char*>(envpStrings[i].c_str()));
	envs.push_back(nullptr);
}
