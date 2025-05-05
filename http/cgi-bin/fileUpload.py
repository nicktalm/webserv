#!/usr/bin/env python3
import os
import cgi
import cgitb

# Aktiviert schönes Debugging (im Browser bei Fehlern)
cgitb.enable()

# Setze den Upload-Ordner (achte darauf, dass dieser Schreibrechte hat!)
UPLOAD_DIR = "http/upload"

# Stelle sicher, dass der Upload-Ordner existiert
os.makedirs(UPLOAD_DIR, exist_ok=True)

# Hole das Formular-Datenobjekt
form = cgi.FieldStorage()

# --- Hilfsfunktion für HTML-Antworten ---
def print_html_response(html_content, status="200 OK"):
    """Gibt eine komplette HTTP-Antwort mit dynamischer Content-Length zurück."""
    html_bytes = html_content.encode('utf-8')
    print(f"Status: {status}")
    print("Content-Type: text/html; charset=utf-8")
    print(f"Content-Length: {len(html_bytes)}")
    print()
    print(html_content)

# --- Upload verarbeiten ---
if "file" not in form:
    html_error = "<html><body><h1>Fehler: Keine Datei hochgeladen!</h1></body></html>"
    print_html_response(html_error, status="400 Bad Request")
    exit(1)

file_item = form["file"]
if file_item.filename:
    filename = os.path.basename(file_item.filename)  # Verhindert Path Injection
    filepath = os.path.join(UPLOAD_DIR, filename)

    # Datei speichern
    with open(filepath, "wb") as f:
        f.write(file_item.file.read())

    # Erfolgreiche Antwort
    html_success = f"""<html>
<head><title>Upload Erfolgreich</title></head>
<body>
    <h1>Datei erfolgreich hochgeladen!</h1>
    <p>Gespeichert als: {filename}</p>
</body>
</html>"""
    print_html_response(html_success)
else:
    html_error = "<html><body><h1>Fehler: Keine Datei gewählt!</h1></body></html>"
    print_html_response(html_error, status="400 Bad Request")
