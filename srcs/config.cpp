/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 14:35:17 by ntalmon           #+#    #+#             */
/*   Updated: 2025/03/25 09:59:21 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

std::string trim(const std::string& line)
{
	std::string trimmed = line;

	size_t start = 0;
	while (start < trimmed.length() && std::isspace(trimmed[start]))
		++start;
	trimmed.erase(0, start);
	size_t end = trimmed.length();
	while (end > 0 && std::isspace(trimmed[end - 1]))
		--end;
	trimmed.erase(end);
	return trimmed;
}

std::string to_lower(const std::string &str)
{
	std::string lower_str = str;
	std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
	return lower_str;
}

// bool check_config(std::string config_path, std::vector<t_config> &files)
// {
// 	std::ifstream	file(config_path);
// 	std::string		line;
// 	t_config		current_config;
// 	t_location		current_location;
// 	bool			in_server_block = false;
// 	bool			in_location_block = false;
// 	bool			expect_server_brace = false;
// 	bool			expect_location_brace = false;

// 	if (!file.is_open())
// 	{
// 		std::cerr << "Error opening file" << std::endl;
// 		return false;
// 	}

// 	while (std::getline(file, line))
// 	{
// 		line = trim(line);
// 		if (line.empty() || line[0] == '#')
// 			continue;
// 		std::string lower_line = to_lower(line); // Groß-/Kleinschreibung ignorieren

// 		std::cout << "Processing line: " << line << std::endl;

// 		if (lower_line == "server")
// 		{
// 			expect_server_brace = true;
// 			continue;
// 		}
// 		if (lower_line.find("location") == 0)
// 		{
// 			expect_location_brace = true;
// 			continue;
// 		}

// 		if (expect_server_brace && line == "{")
// 		{
// 			in_server_block = true;
// 			current_config = t_config();
// 			std::cout << "Entering server block" << std::endl;
// 			expect_server_brace = false;
// 			continue;
// 		}

// 		if (expect_location_brace && line == "{")
// 		{
// 			in_location_block = true;
// 			std::cout << "Entering location block" << std::endl;
// 			expect_location_brace = false;
// 			continue;
// 		}

// 		if (line == "}")
// 		{
// 			if (in_location_block)
// 			{
// 				in_location_block = false;
// 				std::cout << "Exiting location block" << std::endl;
// 			}
// 			else if (in_server_block)
// 			{
// 				files.push_back(current_config);
// 				in_server_block = false;
// 				std::cout << "Exiting server block" << std::endl;
// 			}
// 			continue;
// 		}

// 		if (in_server_block)
// 		{
// 			std::istringstream iss(line);
// 			std::string key, value;
// 			iss >> key >> value;

// 			if (in_location_block)
// 			{
// 				if (key == "index")
// 					current_config.location.index = value;
// 				else if (key == "root")
// 					current_config.location.root = value;
// 				else if (key == "error_page")
// 				{
// 					std::string error_code, error_path;
// 					iss >> error_code >> error_path;
// 					current_config.location.error_page.insert({error_code, error_path});
// 				}
// 			}
// 			else
// 			{
// 				if (key == "server_name")
// 					current_config.server_name = value;
// 				else if (key == "index")
// 					current_config.index = value;
// 				else if (key == "listen")
// 					current_config.port = std::stoi(value);
// 				else if (key == "root")
// 					current_config.root = value;
// 			}
// 		}
// 	}

// 	std::cout << "Parsed Configurations:\n";
// 	for (size_t i = 0; i < files.size(); ++i)
// 	{
// 		std::cout << "----------------------------------------\n";
// 		std::cout << "Server " << i + 1 << ":\n";
// 		std::cout << "  server_name: " << files[i].server_name << "\n";
// 		std::cout << "  index: " << files[i].index << "\n";
// 		std::cout << "  port: " << files[i].port << "\n";
// 		std::cout << "  root: " << files[i].root << "\n";
// 		std::cout << "  Location index: " << files[i].location.index << "\n";
// 		std::cout << "  Location root: " << files[i].location.root << "\n";
// 		for (auto &ep : files[i].location.error_page)
// 		{
// 			std::cout << "  Error page " << ep.first << " -> " << ep.second << "\n";
// 		}
// 	}
// 	std::cout << "----------------------------------------\n";

// 	return true;
// }
