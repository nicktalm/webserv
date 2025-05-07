#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "server.hpp"

// ANSI-Farben
#define COLOR_RESET		"\033[0m"
#define COLOR_INFO		"\033[1;34m"
#define COLOR_WARN		"\033[1;33m"
#define COLOR_ERROR		"\033[1;31m"
#define COLOR_REQUEST	"\033[1;36m"
#define COLOR_SUCCESS	"\033[1;32m"

// Emojis
#define ICON_INFO			"ℹ️"
#define ICON_WARN			"✋🏼"
#define ICON_ERROR		"❌"
#define ICON_REQUEST	"❓"
#define ICON_SUCCESS	"✅"

#define INFO			0;
#define WARN			1;
#define ERROR			2;
#define REQUEST			3;
#define SUCCESS			4;

// Log-Funktion
template <typename... Args>
void log(int level, const std::string& format, Args... args)
{
	// Zeitstempel
	auto now = std::time(nullptr);
	std::tm* tm_ptr = std::localtime(&now);
	std::ostringstream time_stream;
	time_stream << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");

	// Farben & Icons
	const char* color = COLOR_RESET;
	const char* icon = "";
	const char* label = "";

	switch (level) {
		case 0 : color = COLOR_INFO;    icon = ICON_INFO;    label = "INFO";     break;
		case 1 : color = COLOR_WARN;    icon = ICON_WARN;    label = "WARN";     break;
		case 2 : color = COLOR_ERROR;   icon = ICON_ERROR;   label = "ERROR";    break;
		case 3 : color = COLOR_REQUEST; icon = ICON_REQUEST; label = "REQUEST";  break;
		case 4 : color = COLOR_SUCCESS; icon = ICON_SUCCESS; label = "SUCCESS"; break;
	}

	// Formatierung der Nachricht
	std::ostringstream message_stream;
	(message_stream << ... << args);

	// Ausgabe
	std::cout << "[" << time_stream.str() << "] "
			  << color << "[" << icon << " " << label << "] " << format << " " << message_stream.str() << COLOR_RESET << std::endl;
};


namespace utils
{
	// parsing returns the servers created by the config file
	std::vector<std::unique_ptr<Server>>	parsing(int argc, char **argv);

	// reads file and return the input as an std::string
	bool			readFile(std::string input, std::string &body);

	// returns the current date
	std::string	getDate(void);

	// parsing the MIME file
	void	parseMIME(void);

	// maps all the MIME Types
	extern std::map<std::string, std::string>	MIMETypes;

	// Template for autoindex
	const std::string	test = R"(lol
wtf als ob das geht)";

	const std::string autoindexHead = R"(<!DOCTYPE html>
<html lang="de">
<head>
		<meta charset="UTF-8">
		<title>Index of {{path}}</title>
		<style>
				body {
						font-family: Arial, sans-serif;
						background-color: #f7f7f7;
						padding: 40px;
				}
				h1 {
						color: #333;
				}
				table {
						width: 100%;
						border-collapse: collapse;
						background: white;
						box-shadow: 0 2px 5px rgba(0,0,0,0.1);
				}
				th, td {
						padding: 12px 16px;
						text-align: left;
						border-bottom: 1px solid #eee;
				}
				a {
						text-decoration: none;
						color: #0077cc;
				}
				a:hover {
						text-decoration: underline;
				}
				.directory {
						font-weight: bold;
				}
		</style>
</head>
<body>

		<h1>Index of {{path}}</h1>
		<table>
				<tr>
						<th>name</th>
						<th>size</th>
						<th>last change</th>
						<th>delete</th>
				</tr>)";


const std::string autoindexBody = R"(</table>

</body>
<script>
	function deleteFile(filename, button) {
		fetch(`${filename}`, { method: 'DELETE' })
			.then(response => {
				if (response.ok) {
					const row = button.closest("tr");
					row.remove();
					alert(`Deleted: ${filename}`);
				} else {
					alert(`Failed to delete ${filename}`);
				}
			})
			.catch(error => {
				console.error("Error:", error);
				alert("Something went wrong.");
			});
	}
</script>
</html>)";

}
