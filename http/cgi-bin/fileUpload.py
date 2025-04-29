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

# Antwort-Header ausgeben
print("HTTP/1.1 200 OK")  # Status muss "HTTP/1.1 200 OK" sein
print("Content-Type: text/html")

# Verarbeite den Upload
if "file" not in form:
    print("Content-Length: 67")
    print()
    print("<html><body><h1>Fehler: Keine Datei hochgeladen!</h1></body></html>")
    exit(1)

file_item = form["file"]
if file_item.filename:
    filename = os.path.basename(file_item.filename)  # Verhindert Path Injection
    filepath = os.path.join(UPLOAD_DIR, filename)

    with open(filepath, "wb") as f:
        f.write(file_item.file.read())

    # Erfolgsantwort
    print("Content-Length: 158")
    print()
    print(f"""<html>
<head><title>Upload Erfolgreich</title></head>
<body>
    <h1>Datei erfolgreich hochgeladen!</h1>
    <p>Gespeichert als: {filename}</p>
</body>
</html>""")
else:
    print("Content-Length: 63")
    print()
    print("<html><body><h1>Fehler: Keine Datei gewählt!</h1></body></html>")
