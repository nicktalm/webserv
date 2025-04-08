#!/usr/bin/env python3

import os
import sys

# Output HTTP headers
print("Content-Type: text/html\r\n\r\n")

# Output the content
print("<html>")
print("<head><title>Debugging CGI</title></head>")
print("<body>")
print("<h1>CGI Script Debugging</h1>")

# Print environment variables
print("<h2>Environment Variables</h2><pre>")
for key, value in os.environ.items():
    print(f"{key}: {value}")
print("</pre>")

# Print POST data
print("<h2>POST Data</h2><pre>")
content_length = os.environ.get("CONTENT_LENGTH")
if content_length:
    post_data = sys.stdin.read(int(content_length))
    print(post_data)
else:
    print("No POST data received.")
print("</pre>")

print("</body>")
print("</html>")