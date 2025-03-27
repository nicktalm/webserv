/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 14:35:17 by ntalmon           #+#    #+#             */
/*   Updated: 2025/03/27 16:15:19 by ntalmon          ###   ########.fr       */
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
		line = line.substr(0, line.find(";")); // Semikolons entfernen
		if (line.empty() || line[0] == '#')
			continue;
		std::string lower_line = to_lower(line);

		// std::cout << "Processing line: " << line << std::endl;

		if (lower_line == "server") { 
			expect_server_brace = true; 
			continue; 
		}
		if (lower_line.find("location") == 0) {
			std::istringstream iss(line);
			std::string location_keyword, location_path;
			iss >> location_keyword >> location_path;  // "location" und den Pfad extrahieren`
			expect_location_brace = true;
			current_location = t_location();  // Neues Location-Objekt initialisieren
			current_location.path = location_path;  // Pfad speichern

			continue;
		}
		if (expect_server_brace && line == "{")
		{
			in_server_block = true;
			current_config = t_config();  // Neues Server-Objekt starten
			std::cout << "Entering server block" << std::endl;
			expect_server_brace = false;
			continue;
		}

		if (expect_location_brace && line == "{")
		{
			in_location_block = true;
			// Entferne die erneute Initialisierung von current_location hier
			std::cout << "Entering location block" << std::endl;
			expect_location_brace = false;
			continue;
		}

		if (line == "}")
		{
			if (in_location_block)
			{
				current_config.locations.push_back(current_location);
				in_location_block = false;
				std::cout << "Exiting location block" << std::endl;
			}
			else if (in_server_block)
			{
				files.push_back(current_config);
				in_server_block = false;
				std::cout << "Exiting server block" << std::endl;
			}
			continue;
		}
		if (in_server_block)
		{
			std::istringstream iss(line);
			std::string key;
			iss >> key;

			if (in_location_block)
			{
				if (key == "index")
					iss >> current_location.index;
				else if (key == "root")
					iss >> current_location.root;
				else if (key == "limit_except")
				{
					std::string method;
					while (iss >> method)
						current_location.methods.push_back(method);
				}
				else if (key == "client_max_body_size")
				{
					std::string value;
					iss >> value;
					if (!value.empty())
					{
						long zwischenwert = std::stoi(value.substr(0, value.size() - 1));
						current_location.max_size_unit = value.back();
						std::cout << YELLOW << "Max size unit: " << current_location.max_size_unit << RESET << std::endl;
						std::cout << YELLOW << "Zwischenwert: " << zwischenwert << RESET << std::endl;
						if (current_location.max_size_unit == 'K')
							current_location.max_size_location = zwischenwert * 1024;
						else if (current_location.max_size_unit == 'M')
							current_location.max_size_location = zwischenwert * 1024 * 1024;
						else if (current_location.max_size_unit == 'G')
							current_location.max_size_location = zwischenwert * 1024 * 1024 * 1024;
					}
					std::cout << YELLOW << "WERT: " << current_location.max_size_location << RESET << std::endl;
				}
			}
			else
			{
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
				{
					std::string value;
					iss >> value;
					if (!value.empty())
					{
						long zwischenwert2 = std::stoi(value.substr(0, value.size() - 1));
						current_config.max_size_unit_server = value.back();
						std::cout << YELLOW << "Max size unit: " << current_config.max_size_unit_server << RESET << std::endl;
						std::cout << YELLOW << "Zwischenwert2: " << zwischenwert2 << RESET << std::endl;
						if (current_config.max_size_unit_server == 'K')
							current_config.max_size_server = zwischenwert2 * 1024;
						else if (current_config.max_size_unit_server == 'M')
							current_config.max_size_server = zwischenwert2 * 1024 * 1024;
						else if (current_config.max_size_unit_server == 'G')
							current_config.max_size_server = zwischenwert2 * 1024 * 1024 * 1024;
					}
					std::cout << YELLOW << "WERT: " << current_config.max_size_server << RESET << std::endl;
				}
			}
		}
	}

	// Debug-Ausgabe nach dem Parsen
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
			std::cout << "  Location path: " << loc.path << "\n";
			std::cout << "  Location index: " << loc.index << "\n";
			std::cout << "  Location root: " << loc.root << "\n";
			std::cout << "  Location methods:\n";
			for (const auto& method : loc.methods)
				std::cout << "    " << method << "\n";
		}
	}
	std::cout << "----------------------------------------\n";

	return true;
}