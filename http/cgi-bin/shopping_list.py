#!/usr/bin/env python3

import sys
import os
import urllib.parse

USERS_FILE = "users/users.txt"
DATA_DIR = "users/data_shopping"

# Cookie auslesen
cookie_header = os.environ.get("HTTP_COOKIE", "").strip()
username = ""
if "user=" in cookie_header:
    username = cookie_header.split("user=")[1].split(";")[0]

# POST-Daten lesen
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
post_data = sys.stdin.read(content_length) if content_length > 0 else ""
params = urllib.parse.parse_qs(post_data)
shopping_list = params.get('shopping_list', [''])[0]  # <-- Unterstrich beachten!


# Hilfsfunktion: saubere Response senden
def send_response(body, set_cookie=None):
    print("Status: 200 OK")
    if set_cookie:
        print(f"Set-Cookie: {set_cookie}")
    print("Content-Type: text/html")
    print(f"Content-Length: {len(body.encode('utf-8'))}")
    print("\r\n" + body)

# Benutzer prüfen
user_exists = False
if os.path.exists(USERS_FILE):
    with open(USERS_FILE, "r") as file:
        for line in file:
            if line.strip().split(":")[0] == username:
                user_exists = True
                break

if not user_exists:
    body = """<html>
<head><title>Error</title></head>
<body>
<h1>Error: User does not exist</h1>
<a href="/websites/Shoppen.html">Go back</a>
</body>
</html>"""
    send_response(body, f"user={username}; Path=/; HttpOnly")
    sys.exit(0)

# Alte Items lesen
shopping_list_file = os.path.join(DATA_DIR, f"{username}_shopping_list.txt")
existing_items = []
if os.path.exists(shopping_list_file):
    with open(shopping_list_file, "r") as file:
        content = file.read().strip()
        if content:
            existing_items = content.split(", ")

# Neue Items verarbeiten
if shopping_list:
    new_items = shopping_list.split(", ")
    combined_items = existing_items + [item for item in new_items if item and item not in existing_items]
else:
    combined_items = existing_items

# Items speichern
with open(shopping_list_file, "w") as file:
    file.write(", ".join(combined_items))


# HTML Body vorbereiten
existing_html = "<ul>" + "".join(f"<li>{item}</li>" for item in existing_items) + "</ul>" if existing_items else "<p>No items added yet</p>"
new_html = "<ul>" + "".join(f"<li>{item.strip()}</li>" for item in shopping_list.split(",") if item.strip()) + "</ul>" if shopping_list else "<p>No items in your shopping list</p>"

body = f"""<html>
<head>
<title>Shopping List</title>
<style>
body {{ font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; }}
.container {{ max-width: 600px; margin: 50px auto; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); }}
h1 {{ color: #555; }}
ul {{ list-style-type: disc; padding-left: 20px; }}
li {{ margin: 5px 0; }}
</style>
</head>
<body>
<div class='container'>
<h1>Shopping Items added for {username}</h1>

<h2>Existing items:</h2>
{existing_html}

<h2>Newly added items:</h2>
{new_html}

<a href="/websites/Shoppen.html">Go back</a>
</div>
</body>
</html>"""

send_response(body, f"user={username}; Path=/; HttpOnly")
