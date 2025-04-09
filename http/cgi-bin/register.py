#!/usr/bin/env python3

import os
import sys

# File to store registered users
USER_FILE = "./http/users/users.txt"  # Replace with the actual path

# Output HTTP headers
print("HTTP/1.1 200 OK")
print("Content-Type: text/html\r\n\r\n")

# Output the content
print("<html>")
print("<body>")

print("""
	<head>
		<title>Registration</title>
		<style>
			.btn{
			color:white;
			border: 2px solid black;
			border-radius: 12px;
			background-color: black;
			padding: 10px;
			}
		</style>
	</head>
""")

# Extract username and password from script arguments
if len(sys.argv) > 2:
    username = sys.argv[1]
    password = sys.argv[2]

    # Check if the username already exists
    try:
        with open(USER_FILE, "r") as file:
            users = file.readlines()
            for user in users:
                existing_username = user.split(":")[0].strip()
                if existing_username == username:
                    print("<h1>Registration Failed</h1>")
                    print(f"<p>Username '{username}' is already taken.</p>")
                    print("</body></html>")
                    sys.exit(0)
    except FileNotFoundError:
        # If the file doesn't exist, proceed with registration
        pass

    # Append the new username and password to the file
    try:
        with open(USER_FILE, "a") as file:
            file.write(f"{username}:{password}\n")
        print("<h1>Registration Successful</h1>")
        print(f"<p>Welcome, {username}!</p>")
        print('<a class="btn" href="/index.html">Go to main page</a>')
    except Exception as e:
        print("<h1>Registration Failed</h1>")
        print(f"<p>Error: {e}</p>")
else:
    print("<h1>Registration Failed</h1>")
    print("<p>Username and password not provided.</p>")

print("</body>")
print("</html>")