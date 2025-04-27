/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 12:23:41 by lglauch           #+#    #+#             */
/*   Updated: 2025/04/27 20:06:15 by lbohm            ###   ########.fr       */
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
std::string Server::execute_cgi(Client &client, std::string path)
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
		std::map<std::string, std::string> params = parseBody(client.getBody());
		std::string shoppingList = decodeURIcomponent(params["shopping_list"]);
		std::vector<std::string> envpStrings;
		std::string cookie = client.getHeader()["Cookie"];
		// QUERY_STRING	Daten nach dem ? in der URL (bei GET)
		// SCRIPT_NAME	Pfad zum Skript (relativ zum Root)
		// PATH_INFO	Extra-Path-Info nach dem Skriptnamen
		// PATH_TRANSLATED	Physischer Pfad zur Datei aus PATH_INFO
		// REMOTE_ADDR	IP-Adresse des Clients
		// REMOTE_HOST	(wenn DNS-Auflösung aktiv ist)
		// HTTP_USER_AGENT	User-Agent des Browsers
		// HTTP_COOKIE	Cookies, wenn vorhanden
		// HTTP_REFERER	Referrer-URL
		// HTTP_ACCEPT	Was der Client akzeptiert (z. B. HTML, JSON)
		envpStrings.push_back("REQUEST_METHOD=" + client.getMethod());
		envpStrings.push_back("CONTENT_TYPE" + client.getHeader()["Content-Type"]);
		envpStrings.push_back("CONTENT_LENGTH" + client.getHeader()["Content-Length"]);
		envpStrings.push_back("SERVER_NAME" + _config.server_name);
		envpStrings.push_back("SERVER_PORT" + _config.port);
		envpStrings.push_back("SERVER_PROTOCOL" + client.getProtocol());
		envpStrings.push_back("" + );
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