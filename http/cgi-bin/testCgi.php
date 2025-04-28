#!/usr/bin/env php
<?php
// Beginne den Output und den Header
header("HTTP/1.1 200 OK");
header("Content-type: text/html");  // Setzt den Content-Type für HTML
header("Cache-Control: no-cache");  // Verhindert Caching der Antwort
header("Date: " . gmdate("D, d M Y H:i:s") . " GMT");  // Das aktuelle Datum und Uhrzeit
header("Server: PHP CGI Server");  // Setzt den Server-Header
header("Connection: close");  // Verbindungsheader

// Body der Antwort wird jetzt generiert und gespeichert
$html_body = <<<HTML
<html>
<head><title>Test CGI Script</title></head>
<body>
<h1>Willkommen auf unserem CGI-Server!</h1>
HTML;

// Holen des GET-Parameters 'name' aus der URL
$name = isset($_GET['name']) ? htmlspecialchars($_GET['name']) : "Gast";  // Wenn kein Parameter 'name', wird 'Gast' verwendet

// Ausgabe der personalisierten Nachricht
$html_body .= "<p>Hallo, $name!</p>";
$html_body .= "<p>Dies ist eine CGI-Antwort, die den Header und den Body enthält.</p>";

// Optional: Zeige die Umgebungsvariablen (nützlich für Debugging)
$html_body .= "<h2>Umgebungsvariablen:</h2><ul>";
foreach ($_SERVER as $key => $value) {
    $html_body .= "<li><strong>$key:</strong> $value</li>";
}
$html_body .= "</ul>";

// Schließe das HTML-Dokument
$html_body .= "</body></html>";

// Jetzt, wo der Body fertig ist, können wir den Content-Length berechnen
$content_length = strlen($html_body);

// Setze den Content-Length-Header
header("Content-Length: $content_length");

// Gib den Body aus
echo $header;
echo $html_body;
?>
