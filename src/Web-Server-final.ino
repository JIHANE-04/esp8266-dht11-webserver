/**
 * ============================================================
 *  Station de Mesure Température & Humidité
 *  Matériel : NodeMCU ESP8266 (ESP-12E) + Capteur DHT11
 *  Interface : Serveur web asynchrone accessible via Wi-Fi
 *  Auteur    : JIHANE-04
 * ============================================================
 *
 *  Bibliothèques requises (Arduino Library Manager) :
 *    - ESP8266WiFi        (incluse avec le board ESP8266)
 *    - ESPAsyncTCP        (https://github.com/me-no-dev/ESPAsyncTCP)
 *    - ESPAsyncWebServer  (https://github.com/me-no-dev/ESPAsyncWebServer)
 *    - Adafruit_Sensor
 *    - DHT sensor library (Adafruit)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// ============================================================
//  Configuration Wi-Fi
//  Remplacez par vos propres identifiants réseau
// ============================================================
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";

// ============================================================
//  Configuration du capteur DHT11
//  Broche : D1 sur NodeMCU = GPIO5
// ============================================================
#define DHTPIN  5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ============================================================
//  Variables globales — mesures courantes
// ============================================================
float temperature = 0.0;
float humidity    = 0.0;

// ============================================================
//  Serveur web asynchrone — port 80
// ============================================================
AsyncWebServer server(80);

// Intervalle de rafraîchissement des mesures (ms)
unsigned long previousMillis = 0;
const long    interval       = 10000;  // 10 secondes

// ============================================================
//  Page HTML embarquée (stockée en mémoire flash)
// ============================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Température & Humidité</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css">
  <style>
    html {
      font-family: Arial;
      display: inline-block;
      margin: 0 auto;
      text-align: center;
    }
    h2  { font-size: 3.0rem; }
    p   { font-size: 3.0rem; }
    .units      { font-size: 1.2rem; }
    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>🌡️ Station DHT11 - ESP8266</h2>

  <!-- Température -->
  <p>
    <i class="fas fa-thermometer-half fa-beat" style="color:#c70f0f;"></i>
    <span class="dht-labels">Température :</span>
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="dht-labels">°C</sup>
  </p>

  <!-- Humidité -->
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidité :</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="dht-labels">%</sup>
  </p>
</body>

<script>
  // Mise à jour automatique de la température toutes les 10 secondes
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000);

  // Mise à jour automatique de l'humidité toutes les 10 secondes
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 10000);
</script>
</html>
)rawliteral";

// ============================================================
//  Remplacement des balises dans la page HTML
//  %TEMPERATURE% et %HUMIDITY% → valeurs réelles
// ============================================================
String processor(const String& var) {
  if (var == "TEMPERATURE") return String(temperature);
  if (var == "HUMIDITY")    return String(humidity);
  return String();
}

// ============================================================
//  setup() — exécuté une seule fois au démarrage
// ============================================================
void setup() {
  Serial.begin(115200);
  dht.begin();

  // --- Connexion au réseau Wi-Fi ---
  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connecté ! IP locale : ");
  Serial.println(WiFi.localIP());

  // --- Définition des routes HTTP ---

  // Route principale : envoie la page HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Route /temperature : retourne la valeur en texte brut
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(temperature));
  });

  // Route /humidity : retourne la valeur en texte brut
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(humidity));
  });

  // --- Démarrage du serveur ---
  server.begin();
}

// ============================================================
//  loop() — boucle principale (sans blocking delay)
// ============================================================
void loop() {
  unsigned long currentMillis = millis();

  // Lecture du capteur toutes les 10 secondes
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float newTemp = dht.readTemperature();
    float newHum  = dht.readHumidity();

    // Mise à jour de la température si la lecture est valide
    if (!isnan(newTemp)) {
      temperature = newTemp;
      Serial.print("Température : ");
      Serial.print(temperature);
      Serial.println(" °C");
    } else {
      Serial.println("Erreur : lecture température échouée !");
    }

    // Mise à jour de l'humidité si la lecture est valide
    if (!isnan(newHum)) {
      humidity = newHum;
      Serial.print("Humidité    : ");
      Serial.print(humidity);
      Serial.println(" %");
    } else {
      Serial.println("Erreur : lecture humidité échouée !");
    }
  }
}
