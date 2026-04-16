#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===================================================
// CONFIGURATION
// ===================================================
const char* WIFI_SSID     = "ParkingIntelligent";
const char* WIFI_PASSWORD = "parking2026";
const char* SERVER_IP     = "192.168.137.1";
const int   SERVER_PORT   = 8000;
const char* API_KEY       = "une_cle_secrete_longue_pour_esp32_changez_moi";
const char* ESP32_ID      = "node-3";

const int SPOT_ID_1 = 5;  // Place B2
const int SPOT_ID_2 = 6;  // Place B3

const int TRIG_1 = 5,  ECHO_1 = 18;
const int TRIG_2 = 19, ECHO_2 = 21;  // GPIO 21 libre (pas d'OLED)
const int LED_1  = 2,  LED_2  = 4;

const float THRESHOLD_CM = 20.0;

// ===================================================
// ARCHITECTURE
// ===================================================
// COUCHE 1 — AUTONOME : capteurs → LEDs (sans réseau)
// COUCHE 2 — REPORTING : HTTP → Symfony (best-effort, timeout 1s)
// RÉSEAU : Hotspot Windows 192.168.137.1
//          SSID: ParkingIntelligent | MDP: parking2026
// Pas d'OLED sur ce nœud (GPIO 21 utilisé par ECHO_2)
// ===================================================

void connectWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connexion WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500); Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED)
        Serial.println("\n✓ WiFi : " + WiFi.localIP().toString());
    else
        Serial.println("\n✗ WiFi non connecté");
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

void sendSpotData(int spotId, bool occupied, float distanceCm) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perdu — LEDs autonomes, pas d'envoi");
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
    doc["spotId"]     = spotId;
    doc["distanceCm"] = distanceCm;
    doc["isOccupied"] = occupied;
    doc["esp32Id"]    = ESP32_ID;

    String payload;
    serializeJson(doc, payload);
    int httpCode = http.POST(payload);
    Serial.printf("%s Place %d → %s (%.1fcm) HTTP:%d\n",
        httpCode == 201 ? "✓" : "✗", spotId,
        occupied ? "OCCUPEE" : "LIBRE", distanceCm, httpCode);
    http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Parking Intelligent — node-3 ===");

    pinMode(LED_1, OUTPUT); digitalWrite(LED_1, LOW);
    pinMode(LED_2, OUTPUT); digitalWrite(LED_2, LOW);
    pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
    pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);

    connectWifi();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perdu, reconnexion...");
        WiFi.reconnect();
        delay(2000);
    }

    // 1. Mesures (instantané)
    float d1 = measureDistance(TRIG_1, ECHO_1);
    bool  o1 = (d1 > 0 && d1 < THRESHOLD_CM);

    float d2 = measureDistance(TRIG_2, ECHO_2);
    bool  o2 = (d2 > 0 && d2 < THRESHOLD_CM);

    // 2. LEDs instantanées
    digitalWrite(LED_1, o1 ? HIGH : LOW);
    digitalWrite(LED_2, o2 ? HIGH : LOW);

    // 3. Reporting (best-effort)
    sendSpotData(SPOT_ID_1, o1, d1);
    delay(200);
    sendSpotData(SPOT_ID_2, o2, d2);

    delay(4000);
}
