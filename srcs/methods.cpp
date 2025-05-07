/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   methods.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/29 11:06:10 by lbohm             #+#    #+#             */
/*   Updated: 2025/05/07 23:49:53 by ntalmon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "../include/utils.hpp"
#include "../include/server.hpp"

std::string	Server::handleGET(Client &client)
{
	std::string	body;

	if (!client.getHeaderReady())
	{
		std::stringstream	tmpHeader;

		client.setHeaderReady(true);
		if (!client.getCGI())
		{
			tmpHeader << client.getProtocol() << " " + client.getStatusCode() << " " << client.getErrorMsg(client.getStatusCode()) << "\r\n";
			tmpHeader << "Server: " << this->_config.server_name << "\r\n";
			tmpHeader << "Date: " << utils::getDate() << "\r\n";

			if ((client.getLocationInfo().autoindex && client.getPath().back() == '/') || client.getCGI())
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
			return (tmpHeader.str());
		}
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
	else if (client.getCGI())
		return (this->checkCGI(client));
	else
	{
		client.setReady(true);
		if (!utils::readFile(client.getPath(), body))
			return (client.setStatusCode("404"), handleERROR(client));
	}
	return (body);
}

std::string Server::handlePOST(Client &client)
{
	if (client.getCGI())
		return (this->checkCGI(client));

	client.setStatusCode("405");
	return (this->handleERROR(client));
}

std::string Server::handleDELETE(Client &client)
{
	if (client.getCGI())
		return (this->checkCGI(client));
	std::string path = client.getPath();
	if (std::remove(path.c_str()) != 0)
	{
		int errorCode = errno;
		if (errorCode == ENOENT)
			client.setStatusCode("404");
		else if (errorCode == EACCES)
			client.setStatusCode("403");
		else
			client.setStatusCode("500");
		std::cerr << RED; perror("remove"); std::cerr <<  RESET;
		return (handleERROR(client));
	}
	else
		log(4, "File deleted successfully:", path.c_str());
	client.setReady(true);
	client.setStatusCode("204");
	return (this->handleERROR(client));
	// return "HTTP/1.1 204 No Content\r\n\r\n";
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
		path = client.getLocationInfo().error_page.find(client.getStatusCode())->second;
	else
	{
		if (end != std::string::npos)
			path = errorMsg.substr(end + 2);
	}
	if (end != std::string::npos)
	{
		if (!utils::readFile(path, response.body))
			response.body = "";
	}

	response.start_line = "HTTP/1.1 " + client.getStatusCode() + " " + errorMsg.substr(0, end);
	response.server_name = "Server: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + client.getContentType(path);
	client.setReady(true);
	return (Server::create_response(response));
}
