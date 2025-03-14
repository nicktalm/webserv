#include "../include/webserv.hpp"




int main(int argc, char **argv){
	(void)argv;
	if (argc != 2){
		std::cout << RED << "Wrong number of arguments, try [./webserv configuration_file]" << RESET << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << GREEN << "Starting server..." << RESET << std::endl;
	return EXIT_SUCCESS;
}