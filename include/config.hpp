/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 15:27:29 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/31 15:28:19 by lbohm            ###   ########.fr       */
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
	std::multimap<std::string, std::string> error_page = {};
}	t_location;

typedef struct s_config
{
	int port = 0;
	std::string server_name = "";
	std::string index = "";
	std::string root = "";
	std::vector<t_location> locations;
}	t_config;
