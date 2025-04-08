/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 14:35:17 by ntalmon           #+#    #+#             */
/*   Updated: 2025/04/08 10:37:32 by ntalmon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fstream>
#include <iostream>
#include <sstream>
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

bool check_config(const std::string& config_path, std::vector<t_config>& files)
{
	std::ifstream file(config_path);
	if (!file.is_open())
	{
		std::cerr << "Error: Unable to open file: " << config_path << std::endl;
		return false;
	}

	std::string line;
	t_config current_config;
	t_location current_location;
	bool in_server_block = false, in_location_block = false;
	bool expect_server_brace = false, expect_location_brace = false;

	while (std::getline(file, line))
	{
		line = trim(line);
		line = line.substr(0, line.find(";")); // Remove semicolons
		if (line.empty() || line[0] == '#')
			continue;

		if (process_server_block(line, current_config, in_server_block, expect_server_brace))
			continue;

		if (process_location_block(line, current_location, in_location_block, expect_location_brace))
			continue;

		if (process_closing_brace(line, current_config, current_location, files, in_server_block, in_location_block))
			continue;

		if (in_server_block)
		{
			if (in_location_block)
				process_location_directives(line, current_location);
			else
				process_server_directives(line, current_config);
		}
	}
	if (in_server_block)
	{
		std::cerr << "Error: Missing closing brace for server block" << std::endl;
		return false;
	}
	if (in_location_block)
	{
		std::cerr << "Error: Missing closing brace for location block" << std::endl;
		return false;
	}
	add_default_location(files);
	debug_parsed_configurations(files);
	return true;
}


void add_default_location(std::__1::vector<t_config> & files)
{
	// Füge eine Standard-Location hinzu, falls keine Location mit path "/" existiert
	for (auto& server : files)
	{
		bool hasRootLocation = false;
		for (const auto& location : server.locations)
		{
			if (location.path == "/")
			{
				hasRootLocation = true;
				break;
			}
		}
		// Falls keine Location mit path "/" existiert, füge eine hinzu
		if (!hasRootLocation)
		{
			t_location defaultLocation;
			defaultLocation.path = "/";
			defaultLocation.index = server.index; // Übernehme den Index des Serverblocks
			defaultLocation.root = server.root;  // Übernehme den Root des Serverblocks
			server.locations.push_back(defaultLocation);
		}
	}
}

bool process_server_block(const std::string& line, t_config& current_config, bool& in_server_block, bool& expect_server_brace)
{
	if (to_lower(line) == "server")
	{
		expect_server_brace = true;
		return true;
	}
	if (expect_server_brace && line == "{")
	{
		in_server_block = true;
		current_config = t_config(); // Start a new server block
		expect_server_brace = false;
		return true;
	}
	return false;
}

bool process_location_block(const std::string& line, t_location& current_location, bool& in_location_block, bool& expect_location_brace)
{
	if (to_lower(line).find("location") == 0)
	{
		std::istringstream iss(line);
		std::string location_keyword, location_path;
		iss >> location_keyword >> location_path; // Extract "location" and path
		expect_location_brace = true;
		current_location = t_location(); // Initialize a new location object
		current_location.path = location_path; // Save the path
		return true;
	}
	if (expect_location_brace && line == "{")
	{
		in_location_block = true;
		expect_location_brace = false;
		return true;
	}
	return false;
}

bool process_closing_brace(const std::string& line, t_config& current_config, t_location& current_location, std::vector<t_config>& files, bool& in_server_block, bool& in_location_block)
{
	if (line == "}")
	{
		if (in_location_block)
		{
			current_config.locations.push_back(current_location);
			in_location_block = false;
		}
		else if (in_server_block)
		{
			files.push_back(current_config);
			in_server_block = false;
		}
		return true;
	}
	return false;
}

void process_location_directives(const std::string& line, t_location& current_location)
{
	std::istringstream iss(line);
	std::string key;
	iss >> key;

	if (key == "index")
		iss >> current_location.index;
	else if (key == "autoindex")
	{
		std::string value;
		iss >> value;
		current_location.autoindex = (value == "on");
	}
	else if (key == "root")
		iss >> current_location.root;
	else if (key == "limit_except")
	{
		std::string method;
		while (iss >> method)
			current_location.methods.push_back(method);
	}
	else if (key == "client_max_body_size")
		process_client_max_body_size(iss, current_location.max_size_location, current_location.max_size_unit);
	else if (key == "return")
	{
		std::string status_code, redirect_url;
		iss >> status_code >> redirect_url;
		if (status_code.length() != 3 || status_code[0] != '3' || !std::isdigit(status_code[1]) || !std::isdigit(status_code[2]))
		{
			throw std::runtime_error("Error: Invalid status code in return directive. Must be a 3xx code.");
		}
		current_location.redir = std::make_pair(status_code, redirect_url);
	}
	else if (key == "error_page")
	{
		std::string error_code, error_path;
		while (iss >> error_code >> error_path)
			current_location.error_page.emplace(error_code, error_path);
	}
	else
	{
		throw std::runtime_error("Unknown directive in location block");
	}
}

void process_server_directives(const std::string& line, t_config& current_config)
{
	std::istringstream iss(line);
	std::string key;
	iss >> key;

	if (key == "server_name")
		iss >> current_config.server_name;
	else if (key == "index")
		iss >> current_config.index;
	else if (key == "listen")
	{
		std::string value;
		iss >> value;
		try { current_config.port = std::stoi(value); }
		catch (const std::exception&) { 
			std::cerr << "Error: Invalid port number: " << value << std::endl; 
		}
	}
	else if (key == "root")
		iss >> current_config.root;
	else if (key == "error_page")
	{
		std::string error_code, error_path;
		while (iss >> error_code >> error_path)
			current_config.error_page.emplace(error_code, error_path);
	}
	else if (key == "client_max_body_size")
		process_client_max_body_size(iss, current_config.max_size_server, current_config.max_size_unit_server);
	else
	{
		throw std::runtime_error("Unknown directive in server block");
	}
}

void process_client_max_body_size(std::istringstream& iss, long& max_size, char& unit)
{
	std::string value;
	iss >> value;

	if (value.empty())
		throw std::runtime_error("Error: client_max_body_size is missing a value");

	unit = value.back();
	std::string number_part = value;

	if (unit == 'K' || unit == 'M' || unit == 'G')
		number_part = value.substr(0, value.size() - 1);
	else
		unit = 'B'; // Default to bytes

	if (number_part.empty() || !std::isdigit(number_part[0]))
		throw std::runtime_error("Error: No valid numeric value provided for client_max_body_size");

	long intermediate_value;
	try
	{
		intermediate_value = std::stoi(number_part);
	}
	catch (const std::exception&)
	{
		throw std::runtime_error("Error: Invalid numeric value for client_max_body_size");
	}

	if (unit == 'K')
		max_size = intermediate_value * 1024;
	else if (unit == 'M')
		max_size = intermediate_value * 1024 * 1024;
	else if (unit == 'G')
		max_size = intermediate_value * 1024 * 1024 * 1024;
	else
		max_size = intermediate_value;
}

void debug_parsed_configurations(const std::vector<t_config>& files)
{
	std::cout << "Parsed Configurations:\n";
	for (size_t i = 0; i < files.size(); ++i)
	{
		std::cout << "----------------------------------------\n";
		std::cout << GREEN << "Server " << i + 1 << RESET << ":\n";
		std::cout << "  server_name: " << files[i].server_name << "\n";
		std::cout << "  index: " << files[i].index << "\n";
		std::cout << "  port: " << files[i].port << "\n";
		std::cout << "  root: " << files[i].root << "\n";
		std::cout << "  Max size server: " << files[i].max_size_server << "\n";
		std::cout << BLUE << "  Error pages:\n" << RESET;
		for (const auto& [code, path] : files[i].error_page)
			std::cout << "    " << code << " -> " << path << "\n";
		for (const auto& loc : files[i].locations)
		{
			std::cout << GREEN << "Location:\n" << RESET;
			std::cout << "  Location client_max_body_size: " << loc.max_size_location << "\n";
			std::cout << "  Location autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
			std::cout << "  Location redir: " << loc.redir.first << " | " << loc.redir.second << "\n";
			std::cout << "  Location path: " << loc.path << "\n";
			std::cout << "  Location index: " << loc.index << "\n";
			std::cout << "  Location root: " << loc.root << "\n";
			std::cout << BLUE << "  Location error pages:\n" << RESET;
			for (const auto& [code, path] : loc.error_page)
				std::cout << "    " << code << " -> " << path << "\n";
			std::cout << "  Location methods:\n";
			for (const auto& method : loc.methods)
				std::cout << "    " << method << "\n";
		}
	}
	std::cout << "----------------------------------------\n";
}
