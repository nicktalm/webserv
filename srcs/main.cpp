/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lucabohn <lucabohn@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 12:07:48 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/02 23:16:19 by lucabohn         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <csignal>
#include "../include/server.hpp"
#include "../include/utils.hpp"

std::atomic<bool>	runner = true;

void	signalHandler(int signal)
{
	std::cout << YELLOW << "\rInterrupt signal " << signal << " received" << RESET << std::endl;
	runner = false;
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler); //ctrl-c
	try
	{
		std::vector<Server>	servers;

		servers = utils::parsing(argc, argv);
		while(runner)
		{
			for (auto it = servers.begin(); it != servers.end(); ++it)
			{
				it->run();
			}
		}
	}
	catch (std::exception &e)
	{
		std::cout << RED << e.what() << RESET << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

// TODO signal handler muss alle Clients schliessen
// TODO multiple cgi einbauen
// TODO cookies