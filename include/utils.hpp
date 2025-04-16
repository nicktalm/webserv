#pragma once

#include <string>
#include <vector>
#include <map>
#include "server.hpp"

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
