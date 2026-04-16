#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===================================================
// CONFIGURATION — À MODIFIER SI NÉCESSAIRE
// ===================================================
const char* WIFI_SSID     = "ParkingIntelligent";
const char* WIFI_PASSWORD = "parking2026";
const char* SERVER_IP     = "192.168.137.1";
const int   SERVER_PORT   = 8000;
const char* API_KEY       = "une_cle_secrete_longue_pour_esp32_changez_moi";
const char* ESP32_ID      = "node-3";

const int SPOT_ID = 3;  // Place A3

// Broches capteur
const int TRIG = 5;
const int ECHO = 18;

// Broche LED
const int LED = 2;

const float THRESHOLD_CM = 20.0;

// ===================================================
// ARCHITECTURE
// ===================================================
// COUCHE 1 — AUTONOME : capteur → LED (pas de réseau requis)
// COUCHE 2 — REPORTING : envoi HTTP au serveur (best-effort, timeout 1s)
// RÉSEAU : Hotspot Windows 192.168.137.1
//          SSID: ParkingIntelligent | MDP: parking2026
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

float measureDistance() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long duration = pulseIn(ECHO, HIGH, 30000);
    if (duration == 0) return -1;
    return (duration * 0.034) / 2.0;
}

void sendSpotData(bool occupied, float distanceCm) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perdu — LED autonome, pas d'envoi");
        return;
    }

    HTTPClient http;
    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/sensor/data";

    http.begin(url);
    http.setTimeout(1000);
    http.setConnectTimeout(1000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", API_KEY);

    StaticJsonDocument<200> doc;
    doc["spotId"]     = SPOT_ID;
    doc["distanceCm"] = distanceCm;
    doc["isOccupied"] = occupied;
    doc["esp32Id"]    = ESP32_ID;

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    if (httpCode == 201) {
        Serial.printf("✓ Place A3 → %s (%.1f cm)\n", occupied ? "OCCUPÉE" : "LIBRE", distanceCm);
    } else {
        Serial.printf("✗ Erreur HTTP %d\n", httpCode);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Parking Intelligent — node-3 ===");

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    connectWifi();
}

void loop() {
    // Reconnexion WiFi si nécessaire
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perdu, tentative reconnexion...");
        WiFi.reconnect();
        delay(2000);
    }

    // 1. Mesure (instantané)
    float dist = measureDistance();
    bool occupied = (dist > 0 && dist < THRESHOLD_CM);

    // 2. LED instantanée (indépendant du réseau)
    digitalWrite(LED, occupied ? HIGH : LOW);

    // 3. Reporting serveur (best-effort)
    sendSpotData(occupied, dist);

    delay(4500);
}
