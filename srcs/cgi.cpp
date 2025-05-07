/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/05/07 17:25:04 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <signal.h>
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

	if (!client.handleFd(pipe, pipeIn))
		return (false);
	if (!client.handleFd(pipe, pipeOut))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1]);
		return (false);
	}
	
	if (!client.handleFd(fcntl, pipeIn[0], F_SETFL, O_NONBLOCK))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		return (false);
	}
	if (!client.handleFd(fcntl, pipeIn[1], F_SETFL, O_NONBLOCK))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		return (false);
	}
	if (!client.handleFd(fcntl, pipeOut[0], F_SETFL, O_NONBLOCK))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		return (false);
	}
	if (!client.handleFd(fcntl, pipeOut[1], F_SETFL, O_NONBLOCK))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		return (false);
	}

	client.setChildId(fork());
	if (client.getChildId() == -1)
	{
		std::cerr << RED; perror("fork"); std::cerr << RESET;
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
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
	std::string	scriptPath = client.getPath();
	char *const args[] = {const_cast<char *>(scriptPath.c_str()), nullptr};

	size_t pos = scriptPath.find_last_of('/');
	std::string scriptname = (pos != std::string::npos) ? scriptPath.substr(pos + 1) : scriptPath;
	if (pos != std::string::npos)
	{
		std::string dirPath = scriptPath.substr(0, pos);
		if (chdir(dirPath.c_str()) == -1)
		{
			std::cerr << RED; perror("chdir"); std::cerr << RESET;
			exit(1);
		}
	}
	else
	{
		std::cerr << RED << "Invalid script path" << RESET << std::endl;
		exit(1);
	}

	if (!client.handleFd(dup2, pipeIn[0], STDIN_FILENO))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		exit(1);
	}
	if (!client.handleFd(dup2, pipeOut[1], STDOUT_FILENO))
	{
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		exit(1);
	}

	client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
	if (execve(scriptname.c_str(), args, envp.data()) == -1)
	{
		std::cerr << RED; perror("execve"); std::cerr << RESET;
		exit(1);
	}
}

bool	Server::parentProcess(Client &client, int *pipeIn, int *pipeOut)
{
	if (write(pipeIn[1], client.getBody().c_str(), client.getBody().size()) == -1)
	{
		std::cerr << RED << "Write failed" << RESET << std::endl;
		client.setStatusCode("500");
		client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[0], pipeOut[1]);
		return (false);
	}

	client.handleFds(close, pipeIn[0], pipeIn[1], pipeOut[1]);

	if (!client.handleFd(fcntl, pipeOut[0], F_SETFL, O_NONBLOCK))
	{
		client.handleFd(close, pipeOut[0]);
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
		if (!client.getChildReady())
			client.setTimeToExe(std::chrono::system_clock::now());
		auto	diff = std::chrono::system_clock::now() - client.getTimeToExe();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() >= 10000)
		{
			if (kill(client.getChildId(), SIGTERM) == -1)
			{
				std::cerr << RED; perror("kill"); std::cerr << RESET;
			}
			client.setChildReady(false);
			client.setStatusCode("504");
			return (false);
		}
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
			client.setStatusCode("500");
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
