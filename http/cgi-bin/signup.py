#!/usr/bin/env python3

import os
import sys
import urllib.parse

# File to store registered users
USER_FILE = "users/users.txt"

# POST-Datenlänge herausfinden
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

# POST-Daten von stdin lesen
post_data = sys.stdin.read(content_length)

# POST-Daten parsen
params = urllib.parse.parse_qs(post_data)

# Zugriff auf einzelne Felder
username = params.get('username', [''])[0]
password = params.get('password', [''])[0]

def send_response(body, cookie=None):
    """Hilfsfunktion um HTTP-Header + Body auszugeben."""
    print("Status: 200 OK")
    if cookie:
        print(f"Set-Cookie: {cookie}")
    print("Content-Type: text/html")
    content_len = len(body.encode('utf-8'))
    print(f"Content-Length: {content_len}")
    print("\r\n" + body)

# Validate username and password
if not username or not password:
    body = """<html><body>
<h1>Login Failed</h1>
<p>Username and password not provided.</p>
<a href="/index.html">Go back</a>
</body></html>"""
    send_response(body)
    sys.exit(0)

try:
    with open(USER_FILE, "r") as file:
        users = file.readlines()
        for user in users:
            existing_username, existing_password = user.strip().split(":")
            if existing_username == username and existing_password == password:
                body = f"""<html><body>
<h1>Login Successful</h1>
<p>Welcome back, {username}!</p>
<a href="/index.html">Go to main page</a>
</body></html>"""
                cookie = f"user={username}; Path=/; HttpOnly"
                send_response(body, cookie)
                sys.exit(0)
        # Kein Match gefunden
        body = """<html><body>
<h1>Login Failed</h1>
<p>Invalid username or password.</p>
<a href="/index.html">Try again</a>
</body></html>"""
        send_response(body)
except FileNotFoundError:
    body = """<html><body>
<h1>Login Failed</h1>
<p>No users are registered yet.</p>
<a href="/index.html">Go back</a>
</body></html>"""
    send_response(body)
