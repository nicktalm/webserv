/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lucabohn <lucabohn@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/31 12:07:48 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/03 21:44:06 by lucabohn         ###   ########.fr       */
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
		std::cout << RED << e.what() << RESET << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

// TODO signal handler muss alle Clients schliessen
// TODO multiple cgi einbauen
// TODO cookies

// TODO config file listen muss zu host:port geändert werden oder wir brauchen noch einen host value oder eine check das wir nicht zwei server mit dem selben host haben
// TODO config file error pages mussen in locations und server block funktionieren diese definierten error pages gehoeren nicht zu denn die wir in error_pages habne
// TODO limit client body size muss fuer die location und allgemein gescheckt werden status code 413
// TODO config file redirection einbauen
// TODO getPath/checkPath muss noch ueberarbeitet werden location wird nicht bei längerem path gefunden
// TODO autoindex off und keinen index value status code 403 oder auch wenn allgemein kein index page vorhanden ist
// TODO autoindex on und es gibt eine index page dann html generieren mit allen files und ordnern in der location
// TODO transferEncoding = chunked muss fuer die request gemacht werden
