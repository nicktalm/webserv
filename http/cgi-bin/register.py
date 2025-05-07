#!/usr/bin/env python3

import os
import sys
import urllib.parse

# File to store registered users
USER_FILE = "users/users.txt"  # Replace with the actual path

# Output HTTP headers
print("Status: 200 OK")
print("Content-Type: text/html")

# Extract username and password from environment variables
# Content-Length ermitteln
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

# POST-Daten von stdin lesen
post_data = sys.stdin.read(content_length)

# POST-Daten parsen
params = urllib.parse.parse_qs(post_data)

# Zugriff auf einzelne Felder
username = params.get('username', [''])[0]
password = params.get('password', [''])[0]

html = ""

# Check if username and password are provided
if not username or not password:
    html = """<html>
<body>
<h1>Registration Failed</h1>
<p>Username and password not provided.</p>
<a href="/index.html">Go back</a>
</body>
</html>"""
    content_length = len(html.encode('utf-8'))
    print(f"Content-Length: {content_length}")
    print()
    print(html)
    sys.exit(0)

# Check if the username already exists
try:
    with open(USER_FILE, "r") as file:
        users = file.readlines()
        for user in users:
            existing_username = user.split(":")[0].strip()
            if existing_username == username:
                # Output the HTML content for failure
                html = f"""<html>
<body>
<h1>Registration Failed</h1>
<p>Username '{username}' is already taken.</p>
<a href="/index.html">Go back</a>
</body>
</html>"""
                content_length = len(html.encode('utf-8'))
                print(f"Content-Length: {content_length}")
                print()
                print(html)
                sys.exit(0)
except FileNotFoundError:
    # If the file doesn't exist, proceed with registration
    pass

# Append the new username and password to the file
try:
    with open(USER_FILE, "a") as file:
        file.write(f"{username}:{password}\n")
    # Send the Set-Cookie header
    print(f"Set-Cookie: user={username}; Path=/; HttpOnly")
    # Output the HTML content for success
    html = f"""<html>
<head>
    <title>Registration</title>
    <style>
        .btn {{
            color: white;
            border: 2px solid black;
            border-radius: 12px;
            background-color: black;
            padding: 10px;
        }}
    </style>
</head>
<body>
    <h1>Registration Successful</h1>
    <p>Welcome, {username}!</p>
    <a class="btn" href="/index.html">Go to main page</a>
</body>
</html>"""
    content_length = len(html.encode('utf-8'))
    print(f"Content-Length: {content_length}")
    print()
    print(html)
except Exception as e:
    # Output the HTML content for failure
    html_error = f"""<html>
    <body>
        <h1>Registration Failed</h1>
        <p>Error: {e}</p>
    </body>
    </html>"""
    content_length = len(html_error.encode('utf-8'))  # Use html_error here for content length
    print(f"Content-Length: {content_length}")
    print()
    print(html_error)
