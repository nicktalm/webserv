#!/usr/bin/env python3

import sys
import os
import json

USERS_FILE = "./http/users/users.txt"  
DATA_DIR = "./http/users/data_shopping"

username = sys.argv[1].strip() if len(sys.argv) > 1 else "unknown"
shopping_list = sys.argv[2].strip() if len(sys.argv) > 2 else ""

print("HTTP/1.1 200 OK")
print("Content-Type: text/html\r\n")
print(f"Set-Cookie: user={username}; Path=/; HttpOnly\r\n")

# Check if the user exists in users.txt
user_exists = False
if os.path.exists(USERS_FILE):
    with open(USERS_FILE, "r") as file:
        for line in file:
            line = line.strip()
            if line.split(":")[0] == username:  # Compare the username part
                user_exists = True
                break

# If the user does not exist, return an error message
if not user_exists:
    print("<html>")
    print("<head><title>Error</title></head>")
    print("<body>")
    print("<h1>Error: User does not exist</h1>")
    print('<a href="/websites/Shoppen.html">Go back</a>')
    print("</body>")
    print("</html>")
    sys.exit(0)

# If the user exists, save the shopping list to a file
shopping_list_file = os.path.join(DATA_DIR, f"{username}_shopping_list.txt")

# Write the shopping list to the file
existing_items = []
if os.path.exists(shopping_list_file):
    with open(shopping_list_file, "r") as file:
        existing_items = file.read().split(", ")

# Combine the existing items with the new items
if shopping_list:
    new_items = shopping_list.split(", ")
    combined_items = existing_items + [item for item in new_items if item not in existing_items]  # Avoid duplicates
else:
    combined_items = existing_items

# Write the updated shopping list back to the file
with open(shopping_list_file, "w") as file:
    file.write(", ".join(combined_items))

# Generate the HTML response
print("<html>")
print("<head>")
print("<title>Shopping List</title>")
print("<style>")
print("body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; }")
print(".container { max-width: 600px; margin: 50px auto; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); }")
print("h1 { color: #555; }")
print("ul { list-style-type: disc; padding-left: 20px; }")
print("li { margin: 5px 0; }")
print("</style>")
print("</head>")
print("<body>")
print("<div class='container'>")
print(f"<h1>Shopping List for {username}</h1>")

if shopping_list:
    items = shopping_list.split(",")
    print("<ul>")
    for item in items:
        print(f"<li>{item.strip()}</li>")
    print("</ul>")
else:
    print("<p>No items in your shopping list.</p>")

print('<a href="/websites/Shoppen.html">Go back</a>')
print("</div>")
print("</body>")
print("</html>")