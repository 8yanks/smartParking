#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================================================
// CONFIGURATION — À MODIFIER AVANT DE FLASHER
// ===================================================
const char* WIFI_SSID     = "NOM_DE_VOTRE_WIFI";
const char* WIFI_PASSWORD = "MOT_DE_PASSE_WIFI";
const char* SERVER_IP     = "192.168.1.100";  // IP du PC Symfony
const int   SERVER_PORT   = 8000;
const char* API_KEY       = "une_cle_secrete_longue_pour_esp32_changez_moi";
const char* ESP32_ID      = "node-2";

// Places gérées par ce nœud
const int SPOT_ID_1 = 3;  // Place A3
const int SPOT_ID_2 = 4;  // Place B1

// Broches capteur 1 (place A3)
const int TRIG_1 = 5;
const int ECHO_1 = 18;

// Broches capteur 2 (place B1)
const int TRIG_2 = 19;
const int ECHO_2 = 23;  // GPIO 21 réservé pour OLED SDA

// Broches LEDs
const int LED_1 = 2;
const int LED_2 = 4;

// OLED (I2C : SDA=21, SCL=22 par défaut sur ESP32)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Seuil de détection
const float THRESHOLD_CM = 20.0;
// ===================================================

// ===================================================
// ARCHITECTURE : AUTONOMIE vs REPORTING
// ===================================================
// Ce nœud fonctionne en 2 couches indépendantes :
//
// COUCHE 1 — AUTONOME (pas de réseau requis)
//   Capteurs → mesure distance → LEDs allumées/éteintes
//   Réponse : ~50ms, fonctionne même si WiFi est coupé
//
// COUCHE 2 — REPORTING (best-effort, timeout 1s)
//   Envoi des données au serveur Symfony local
//   Si le serveur est lent ou injoignable : on skip, pas de blocage
// ===================================================

void connectWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connexion WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi : " + WiFi.localIP().toString());
    } else {
        Serial.println("\n✗ WiFi non connecté");
    }
}

float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000);
    if (duration == 0) return -1;
    return (duration * 0.034) / 2.0;
}

void updateDisplay(bool occ1, bool occ2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Titre
    display.setCursor(0, 0);
    display.println("== PARKING NOEUD 2 ==");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Place 1
    display.setCursor(0, 16);
    display.printf("Place A3 (ID %d):", SPOT_ID_1);
    display.setCursor(0, 26);
    display.setTextSize(2);
    display.println(occ1 ? "OCCUPEE" : "LIBRE");
    display.setTextSize(1);

    // Séparateur
    display.drawLine(0, 42, 127, 42, SSD1306_WHITE);

    // Place 2
    display.setCursor(0, 46);
    display.printf("Place B1 (ID %d):", SPOT_ID_2);
    display.setCursor(0, 56);
    display.println(occ2 ? "OCCUPEE" : "LIBRE  ");

    display.display();
}

void sendSpotData(int spotId, bool occupied, float distanceCm) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠ WiFi perdu — LED autonomes, pas d'envoi serveur");
        return; // On ne bloque pas, on skip juste l'envoi
    }

    HTTPClient http;
    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/sensor/data";

    http.begin(url);
    http.setTimeout(1000); // 1 seconde maximum
    http.setConnectTimeout(1000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", API_KEY);

    StaticJsonDocument<200> doc;
    doc["spotId"]     = spotId;
    doc["distanceCm"] = distanceCm;
    doc["isOccupied"] = occupied;
    doc["esp32Id"]    = ESP32_ID;

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);

    if (httpCode == 201) {
        Serial.printf("✓ Place %d → %s (%.1f cm)\n", spotId, occupied ? "OCCUPÉE" : "LIBRE", distanceCm);
    } else {
        Serial.printf("✗ Place %d → Erreur HTTP %d\n", spotId, httpCode);
    }

    http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Parking Intelligent — node-2 ===");

    // LEDs
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);

    // Capteurs
    pinMode(TRIG_1, OUTPUT);
    pinMode(ECHO_1, INPUT);
    pinMode(TRIG_2, OUTPUT);
    pinMode(ECHO_2, INPUT);

    // OLED
    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Erreur OLED — vérifiez le câblage");
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Démarrage...");
        display.display();
    }

    connectWifi();
}

void loop() {
    // Reconnexion WiFi si nécessaire (non-bloquant après 1 tentative)
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perdu, tentative reconnexion...");
        WiFi.reconnect();
        delay(2000);
        // On continue quand même — les LEDs fonctionnent sans WiFi
    }

    // 1. Mesures instantanées (indépendant du réseau)
    float dist1 = measureDistance(TRIG_1, ECHO_1);
    bool occ1   = (dist1 > 0 && dist1 < THRESHOLD_CM);

    float dist2 = measureDistance(TRIG_2, ECHO_2);
    bool occ2   = (dist2 > 0 && dist2 < THRESHOLD_CM);

    // 2. LEDs instantanées (indépendant du réseau)
    digitalWrite(LED_1, occ1 ? HIGH : LOW);
    digitalWrite(LED_2, occ2 ? HIGH : LOW);

    // 3. Affichage OLED (indépendant du réseau)
    updateDisplay(occ1, occ2);

    // 4. Reporting serveur (best-effort, timeout 1s)
    sendSpotData(SPOT_ID_1, occ1, dist1);
    delay(200);
    sendSpotData(SPOT_ID_2, occ2, dist2);

    // 5. Pause avant prochain cycle
    delay(4000);
}
