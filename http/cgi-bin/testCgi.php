#!/usr/bin/php
<?php
// Setze den Status-Header (analog zu "Status: 200 OK" im Python-Skript)
echo "Status: 200 OK\n";
echo "Content-Type: text/html\n";  // Setzt den Content-Type für HTML
echo "Cache-Control: no-cache\n";  // Verhindert Caching der Antwort
echo "Date: " . gmdate("D, d M Y H:i:s") . " GMT\n";  // Aktuelles Datum und Uhrzeit im GMT-Format
echo "Server: PHP CGI Server\n";  // Setzt den Server-Header
echo "Connection: close\n";  // Verbindungs-Header

// Beginn des HTML-Bodys
$html_body = "
<html>
<head><title>Test CGI Script</title></head>
<body>
<h1>Willkommen auf unserem CGI-Server!</h1>
";

// Hole den 'name' GET-Parameter aus der URL (wie im Python-Skript)
$query_string = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '';

// Parsen der Query-String, um den 'name' Parameter zu extrahieren
parse_str($query_string, $query_params);
$name = isset($query_params['name']) ? $query_params['name'] : "Gast";  // Standardwert "Gast" wenn kein Parameter vorhanden

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

// Berechne die Content-Length
$content_length = strlen($html_body);

// Setze den Content-Length-Header
echo "Content-Length: $content_length\n";
echo "\n";  // Leere Zeile zur Trennung der Header vom Body

// Gib den Body aus
echo $html_body;
?>
