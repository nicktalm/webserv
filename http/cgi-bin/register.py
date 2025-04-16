#!/usr/bin/env python3

import os
import sys

# File to store registered users
USER_FILE = "./http/users/users.txt"  # Replace with the actual path

# Output HTTP headers
print("HTTP/1.1 200 OK")
print("Content-Type: text/html")

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
                    # Output the HTML content for failure
                    print("\r\n")  # End of headers
                    print("<html>")
                    print("<body>")
                    print("<h1>Registration Failed</h1>")
                    print(f"<p>Username '{username}' is already taken.</p>")
                    print('<a href="/index.html">Go back</a>')
                    print("</body>")
                    print("</html>")
                    sys.exit(0)
    except FileNotFoundError:
        # If the file doesn't exist, proceed with registration
        pass

    # Append the new username and password to the file
    try:
        with open(USER_FILE, "a") as file:
            file.write(f"{username}:{password}\n")
        # Send the Set-Cookie header
        print(f"Set-Cookie: user={username}; Path=/; HttpOnly\r\n")
        # Output the HTML content for success
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
        print("<h1>Registration Successful</h1>")
        print(f"<p>Welcome, {username}!</p>")
        print('<a class="btn" href="/index.html">Go to main page</a>')
    except Exception as e:
        # Output the HTML content for failure
        print("\r\n")  # End of headers
        print("<html>")
        print("<body>")
        print("<h1>Registration Failed</h1>")
        print(f"<p>Error: {e}</p>")
        print("</body>")
        print("</html>")
else:
    # Output the HTML content for missing arguments
    print("\r\n")  # End of headers
    print("<html>")
    print("<body>")
    print("<h1>Registration Failed</h1>")
    print("<p>Username and password not provided.</p>")
    print('<a href="/index.html">Go back</a>')
    print("</body>")
    print("</html>")