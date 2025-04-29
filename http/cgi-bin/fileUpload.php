<?php
// Aktiviert Fehleranzeige
ini_set('display_errors', 1);
error_reporting(E_ALL);

// Setze den Upload-Ordner (achte darauf, dass dieser Schreibrechte hat!)
$upload_dir = "http/upload";

// Stelle sicher, dass der Upload-Ordner existiert
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0777, true);
}

// Antwort-Header ausgeben
header("HTTP/1.1 200 OK");
header("Content-Type: text/html");

// Prüfe, ob eine Datei hochgeladen wurde
if (!isset($_FILES['file']) || $_FILES['file']['error'] !== UPLOAD_ERR_OK) {
    $error_html = "<html><body><h1>Fehler: Keine Datei hochgeladen!</h1></body></html>";
    
    // Setze den Content-Length Header für den Fehler
    header("Content-Length: " . strlen($error_html));
    
    // Gib die Fehlermeldung aus
    echo $error_html;
    exit;
}

// Hole Dateiinformationen
$filename = basename($_FILES['file']['name']);
$filepath = $upload_dir . "/" . $filename;

// Speichere die hochgeladene Datei
if (move_uploaded_file($_FILES['file']['tmp_name'], $filepath)) {
    $success_html = "<html>
<head><title>Upload Erfolgreich</title></head>
<body>
    <h1>Datei erfolgreich hochgeladen!</h1>
    <p>Gespeichert als: " . htmlspecialchars($filename) . "</p>
</body>
</html>";
    
    // Setze den Content-Length Header für den Erfolg
    header("Content-Length: " . strlen($success_html));
    
    // Gib die Erfolgsnachricht aus
    echo $success_html;
} else {
    $fail_html = "<html><body><h1>Fehler: Datei konnte nicht gespeichert werden!</h1></body></html>";
    
    // Setze den Content-Length Header für den Fehler
    header("Content-Length: " . strlen($fail_html));
    
    // Gib die Fehlermeldung aus
    echo $fail_html;
}
?>
