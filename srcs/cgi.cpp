/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/04/27 18:47:35 by lbohm            ###   ########.fr       */
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
std::string execute_cgi(Client &client, std::string path)
{
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		std::cerr << RED << "Pipe creation failed" << RESET << std::endl;
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}
	
	// if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(pipefd[1], F_SETFL, O_NONBLOCK) == -1)
	// {
		// 	if (pipefd[0] >= 0) close(pipefd[0]);
		// 	if (pipefd[1] >= 0) close(pipefd[1]);
		// 	std::cerr << RED << "fcntl failed" << RESET << std::endl;
		// 	return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
		// }
		
		pid_t pid = fork();
		if (pid == -1)
		{
			close(pipefd[0]);
			close(pipefd[1]);
			std::cerr << RED << "Fork failed" << RESET << std::endl;
			return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
		}
		else if (pid == 0) // Child process
		{
			// std::cout << client.getBody() << std::endl;
			std::map<std::string, std::string> params = parseBody(client.getBody());
			std::string shoppingList = decodeURIcomponent(params["shopping_list"]);
			std::vector<std::string> envpStrings;
			std::string cookie = client.getHeader()["Cookie"];
			envpStrings.push_back("COOKIE=" + cookie);
			envpStrings.push_back("USERNAME=" + params["username"]);
			envpStrings.push_back("PASSWORD=" + params["password"]);
			envpStrings.push_back("SHOPPINGLIST=" + shoppingList);

			std::vector<char*> envp;
			for(size_t i = 0; i < envpStrings.size(); i++)
			{
				envp.push_back(const_cast<char*>(envpStrings[i].c_str()));
			}
			envp.push_back(nullptr);
			dup2(pipefd[1], STDOUT_FILENO);
			char *const args[] = {const_cast<char *>(path.c_str()), nullptr};
			if (execve(path.c_str(), args, envp.data()) == -1)
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
			// std::cout << YELLOW << "CGI:\n" << cgiOutput << std::endl;
			return cgiOutput;
		}
	return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
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
		return execute_cgi(client, final_path);
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