#!/usr/bin/env python3

import cgi
import os
import datetime

# Beginne den Output und den Header
print("Status: 200 OK")  # Dies ist der Status-Header, nicht HTTP/1.1
print("Content-Type: text/html")  # Setzt den Content-Type für HTML
print("Cache-Control: no-cache")  # Verhindert Caching der Antwort
print("Date:", datetime.datetime.now().strftime("%a, %d %b %Y %H:%M:%S GMT"))  # Das aktuelle Datum und Uhrzeit
print("Server: Python CGI Server")  # Setzt den Server-Header
print("Connection: close")  # Verbindungsheader

# Body der Antwort wird jetzt generiert und gespeichert
html_body = """
<html>
<head><title>Test CGI Script</title></head>
<body>
<h1>Willkommen auf unserem CGI-Server!</h1>
"""

# Holen des GET-Parameters 'name' aus der URL
form = cgi.FieldStorage()
name = form.getvalue("name", "Gast")  # Wenn kein Parameter 'name', wird 'Gast' verwendet

# Ausgabe der personalisierten Nachricht
html_body += f"<p>Hallo, {name}!</p>"
html_body += "<p>Dies ist eine CGI-Antwort, die den Header und den Body enthält.</p>"

# Optional: Zeige die Umgebungsvariablen (nützlich für Debugging)
html_body += "<h2>Umgebungsvariablen:</h2><ul>"
for key, value in os.environ.items():
    html_body += f"<li><strong>{key}:</strong> {value}</li>"
html_body += "</ul>"

# Schließe das HTML-Dokument
html_body += "</body></html>"

# Jetzt, wo der Body fertig ist, können wir den Content-Length berechnen
content_length = len(html_body)

# Setze den Content-Length-Header
print(f"Content-Length: {content_length}")
print()  # Leere Zeile, um die Header von der Body-Inhalte zu trennen

# Gib den Body aus
print(html_body)
