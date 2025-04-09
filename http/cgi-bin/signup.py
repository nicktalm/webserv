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
print("<head><title>Login</title></head>")
print("<body>")

# Extract username and password from script arguments
if len(sys.argv) > 2:
    username = sys.argv[1]
    password = sys.argv[2]

    # Check if the username and password combination exists
    try:
        with open(USER_FILE, "r") as file:
            users = file.readlines()
            for user in users:
                existing_username, existing_password = user.strip().split(":")
                if existing_username == username and existing_password == password:
                    print("<h1>Login Successful</h1>")
                    print(f"<p>Welcome back, {username}!</p>")
                    print('<a href="/index.html">Go to main page</a>')
                    print("</body></html>")
                    sys.exit(0)
            # If no match is found
            print("<h1>Login Failed</h1>")
            print("<p>Invalid username or password.</p>")
            print('<a href="/index.html">Try again</a>')
    except FileNotFoundError:
        print("<h1>Login Failed</h1>")
        print("<p>No users are registered yet.</p>")
        print('<a href="/index.html">Go back</a>')
    except Exception as e:
        print("<h1>Login Failed</h1>")
        print(f"<p>Error: {e}</p>")
else:
    print("<h1>Login Failed</h1>")
    print("<p>Username and password not provided.</p>")
    print('<a href="/index.html">Go back</a>')

print("</body>")
print("</html>")