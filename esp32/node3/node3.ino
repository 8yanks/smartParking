#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===================================================
// CONFIGURATION — À MODIFIER AVANT DE FLASHER
// ===================================================
const char* WIFI_SSID       = "NOM_DE_VOTRE_WIFI";
const char* WIFI_PASSWORD   = "MOT_DE_PASSE_WIFI";
const char* SERVER_IP       = "192.168.1.100";   // IP du PC qui fait tourner Symfony
const int   SERVER_PORT     = 8000;
const char* API_KEY         = "une_cle_secrete_longue_pour_esp32_changez_moi";
const char* ESP32_ID        = "node-3";           // Identifiant de ce nœud

// IDs des places gérées par CE nœud
const int SPOT_ID_1 = 5;
const int SPOT_ID_2 = 6;

// Broches capteur 1 (place SPOT_ID_1)
const int TRIG_1 = 5;
const int ECHO_1 = 18;

// Broches capteur 2 (place SPOT_ID_2)
const int TRIG_2 = 19;
const int ECHO_2 = 21;

// Distance seuil : en-dessous = voiture présente
const float THRESHOLD_CM = 20.0;

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
        Serial.println("\n✓ WiFi connecté : " + WiFi.localIP().toString());
    } else {
        Serial.println("\n✗ WiFi non connecté, nouvelle tentative au prochain cycle");
    }
}

float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
    if (duration == 0) return -1; // pas de réponse
    return (duration * 0.034) / 2.0;
}

void sendSpotData(int spotId, bool occupied, float distanceCm) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi déconnecté, reconnexion...");
        connectWifi();
        return;
    }

    HTTPClient http;
    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/sensor/data";

    http.begin(url);
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
    Serial.println("\n=== Parking Intelligent — " + String(ESP32_ID) + " ===");

    pinMode(TRIG_1, OUTPUT);
    pinMode(ECHO_1, INPUT);
    pinMode(TRIG_2, OUTPUT);
    pinMode(ECHO_2, INPUT);

    connectWifi();
}

void loop() {
    // Mesure capteur 1
    float dist1 = measureDistance(TRIG_1, ECHO_1);
    bool occ1   = (dist1 > 0 && dist1 < THRESHOLD_CM);
    sendSpotData(SPOT_ID_1, occ1, dist1);
    delay(500);

    // Mesure capteur 2
    float dist2 = measureDistance(TRIG_2, ECHO_2);
    bool occ2   = (dist2 > 0 && dist2 < THRESHOLD_CM);
    sendSpotData(SPOT_ID_2, occ2, dist2);
    delay(500);

    // Pause avant prochain cycle (~4s + 2x0.5s = ~5s total)
    delay(4000);
}
