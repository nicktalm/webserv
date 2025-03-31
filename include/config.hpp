/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 15:27:29 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/31 15:44:14 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>

typedef struct s_location
{
	bool autoindex = false;
	std::string index = "";
	std::string root = "";
	std::vector<std::string> methods;
	std::string path = "";
	long max_size_location = 0;
	char max_size_unit = ' ';
}	t_location;

typedef struct s_config
{
	int port = 0;
	std::string server_name = "";
	std::string index = "";
	std::string root = "";
	std::multimap<std::string, std::string> error_page = {};
	long max_size_server = 0;
	char max_size_unit_server = ' ';
	std::vector<t_location> locations;
}	t_config;
