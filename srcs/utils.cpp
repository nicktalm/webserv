/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 12:07:23 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/31 13:09:04 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

std::map<std::string, std::string> utils::MIMETypes = {{"", ""}};

std::vector<Server>	utils::parsing(int argc, char **argv)
{
	std::vector<t_config>	files;
	std::vector<Server>		servers;

	if (argc > 2)
		throw std::runtime_error("Wrong number of arguments, try [./webserv configuration_file]");
	else
	{
		std::string config_file;
		if (argc == 2)
			config_file = argv[1];
		else
			config_file = "default/default.conf";
		if (!check_config(config_file, files))
			throw std::runtime_error("Error in config file");
	}
	for (auto config : files)
		servers.emplace_back(config);
	parseMIME();
	return (servers);
}

std::string	utils::readFile(std::string input)
{
	std::ifstream		file(input);
	std::stringstream	buffer;

	if (!file)
		return("");
	buffer << file.rdbuf();
	return (buffer.str());
}

std::string utils::getDate(void)
{
	time_t currentTime;
	struct tm* ti;

	time(&currentTime);
	ti = localtime(&currentTime);
	char timeBuffer[100];
	strftime(timeBuffer, sizeof(timeBuffer), "%a, %d %b %Y %H:%M:%S GMT", ti);
	return (std::string(timeBuffer) + "\r\n");
}

void	utils::parseMIME(void)
{
	std::ifstream				file("srcs/response/MIME-Type.csv");

	if (!file)
		std::cerr << "open failed" << std::endl;
	else
	{
		std::string	line;
		while (std::getline(file, line))
		{
			std::stringstream	linestream(line);
			std::string			key, value;

			if (std::getline(linestream, key, ',') && std::getline(linestream, value))
				MIMETypes[key] = value;
		}
	}
}