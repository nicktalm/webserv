/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 12:07:48 by lbohm             #+#    #+#             */
/*   Updated: 2025/05/08 11:05:06 by ntalmon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <csignal>
#include "../include/server.hpp"
#include "../include/utils.hpp"
#include "fcntl.h"

std::atomic<bool>	runner = true;

void	signalHandler(int signal)
{
	// std::cerr << YELLOW << "\rInterrupt signal " << signal << " received" << RESET << std::endl;
	std::cout << "\r";
	log(0, "Interrupt signal", signal , " received");
	runner = false;
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler); //ctrl-c
	try
	{
		std::vector<std::unique_ptr<Server>>	servers;

		servers = utils::parsing(argc, argv);
		while(runner)
		{
			for (auto it = servers.begin(); it != servers.end(); ++it)
			{
				(*it)->run();
			}
		}
	}
	catch (std::exception &e)
	{
		std::cerr << RED << e.what() << RESET << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
