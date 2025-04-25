#!/usr/bin/env python3

import os
import sys

# File to store registered users
USER_FILE = "./http/users/users.txt"

# Output HTTP headers
print("HTTP/1.1 200 OK")
print("Content-Type: text/html")

# Extract username and password from script arguments
username = os.environ.get("USERNAME", "")
password = os.environ.get("PASSWORD", "")
print("Username: ", username)
print("Password ", password)

    # Validate username and password

if not username or not password:
    print("\r\n")  # End of headers
    print("<html><body>")
    print("<h1>Login Failed</h1>")
    print("<p>Username and password not provided.</p>")
    print('<a href="/index.html">Go back</a>')
    print("</body></html>")

try:
    with open(USER_FILE, "r") as file:
        users = file.readlines()
        for user in users:
            existing_username, existing_password = user.strip().split(":")
            if existing_username == username and existing_password == password:
                # Send the Set-Cookie header to update the cookie
                print(f"Set-Cookie: user={username}; Path=/; HttpOnly\r\n")
                # Output the HTML content for success
                print("<html><body>")
                print("<h1>Login Successful</h1>")
                print(f"<p>Welcome back, {username}!</p>")
                print('<a href="/index.html">Go to main page</a>')
                print("</body></html>")
                sys.exit(0)
        # If no match is found
        print("\r\n")  # End of headers
        print("<html><body>")
        print("<h1>Login Failed</h1>")
        print("<p>Invalid username or password.</p>")
        print('<a href="/index.html">Try again</a>')
        print("</body></html>")
except FileNotFoundError:
    print("\r\n")  # End of headers
    print("<html><body>")
    print("<h1>Login Failed</h1>")
    print("<p>No users are registered yet.</p>")
    print('<a href="/index.html">Go back</a>')
    print("</body></html>")