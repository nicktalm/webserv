/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 15:27:29 by lbohm             #+#    #+#             */
/*   Updated: 2025/05/08 18:31:32 by ntalmon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>

typedef struct s_location
{
	bool autoindex = false;
	char max_size_unit = ' ';
	long max_size_location = 0;
	std::string index = "";
	std::string root = "";
	std::string path = "";
	std::vector<std::string> methods = std::vector<std::string>();
	std::multimap<std::string, std::string> error_page = {};
	std::pair<std::string, std::string>	redir = {};
}	t_location;

typedef struct s_config
{
	char max_size_unit_server = ' ';
	std::string port = "";
	long max_size_server = 0;
	std::string server_name = "";
	std::string index = "";
	std::string root = "";
	std::vector<t_location> locations;
	std::multimap<std::string, std::string> error_page = {};
}	t_config;
