#!/usr/bin/env python3

import cgi
import cgitb
import os

cgitb.enable()  # Zeigt Tracebacks im Browser bei Fehlern

form = cgi.FieldStorage()

def print_html_response(html_content, status="200 OK"):
    """Gibt eine komplette HTTP-Antwort mit dynamischer Content-Length zurück."""
    html_bytes = html_content.encode('utf-8')
    print(f"Status: {status}")
    print("Content-Type: text/html; charset=utf-8")
    print(f"Content-Length: {len(html_bytes)}")
    print()  # Leerzeile nach den Headern
    print(html_content)

# Eingabefeld prüfen
if "json_file" not in form:
    print_html_response("<h1>Fehler: Kein Dateifeld 'json_file' gefunden.</h1>", status="400 Bad Request")
    exit()

fileitem = form["json_file"]

if fileitem.filename:
    filename = os.path.basename(fileitem.filename)

    # Speicherort anpassen!
    upload_dir = "upload"
    save_path = os.path.join(upload_dir, filename)

    try:
        with open(save_path, "wb") as f:
            f.write(fileitem.file.read())

        html = f"""
            <h1>Upload erfolgreich!</h1>
            <p>Dateiname: {filename}</p>
            <p>Gespeichert unter: {save_path}</p>
        """
        print_html_response(html)
    except Exception as e:
        error_html = f"<h1>Fehler beim Speichern:</h1><pre>{e}</pre>"
        print_html_response(error_html, status="500 Internal Server Error")
else:
    print_html_response("<h1>Fehler: Keine Datei ausgewählt.</h1>", status="400 Bad Request")
