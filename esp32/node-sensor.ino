/**
 * Parking Intelligent — Sketch nœud capteur (réutilisé pour #2 et #3)
 *
 * Rôle :
 *  - Mesure 2 places via HC-SR04A
 *  - Allume les LEDs locales si occupées
 *  - POST l'état vers /api/spot/{id}
 *
 * Pour flasher un nœud précis, modifiez la section CONFIGURATION ci-dessous :
 *   - NODE_ID : "node-2" ou "node-3"
 *   - SPOT_ID_A / SPOT_ID_B : (3,4) pour node-2 ; (5,6) pour node-3
 *
 * Dépendances Arduino : WiFi, HTTPClient, ArduinoJson
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================================
// CONFIGURATION — À MODIFIER avant flash
// ============================================================
const char* WIFI_SSID     = "NOM_DE_VOTRE_WIFI";
const char* WIFI_PASSWORD = "MOT_DE_PASSE_WIFI";
const char* SERVER_URL    = "http://192.168.137.1:3000"; // IP du PC sur le hotspot, sans slash final
const char* API_KEY       = "une_cle_secrete_longue_changez_moi";

// >>> Spécifique à chaque nœud — décommenter le bon bloc <<<

// --- Nœud #2 ---
const char* NODE_ID  = "node-2";
const int SPOT_ID_A  = 3;
const int SPOT_ID_B  = 4;

// --- Nœud #3 ---
// const char* NODE_ID  = "node-3";
// const int SPOT_ID_A  = 5;
// const int SPOT_ID_B  = 6;

// ============================================================
// CÂBLAGE GPIO — identique sur tous les nœuds capteurs
// voir esp32/README.md pour le schéma de câblage
// ============================================================
#define TRIG_A 25
#define ECHO_A 26
#define TRIG_B 27
#define ECHO_B 14

#define LED_A 32
#define LED_B 33

// ============================================================
// PARAMÈTRES MÉTIER
// ============================================================
const float DISTANCE_THRESHOLD_CM = 20.0;   // < seuil = voiture présente
const unsigned long LOOP_DELAY_MS = 4500;   // intervalle global du loop

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("\n[%s] Boot capteur\n", NODE_ID);

    pinMode(TRIG_A, OUTPUT); pinMode(ECHO_A, INPUT);
    pinMode(TRIG_B, OUTPUT); pinMode(ECHO_B, INPUT);
    pinMode(LED_A, OUTPUT);  digitalWrite(LED_A, LOW);
    pinMode(LED_B, OUTPUT);  digitalWrite(LED_B, LOW);

    connectWifi();
}

// ============================================================
// WIFI
// ============================================================
void connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] connexion");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] OK %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] echec — retry au prochain envoi");
    }
}

// ============================================================
// CAPTEURS
// ============================================================
float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30 ms
    if (duration == 0) return -1.0;
    return (duration * 0.034) / 2.0;
}

// ============================================================
// API — POST état d'une place
// ============================================================
void postSpotStatus(int spotId, bool occupied, float distance) {
    if (WiFi.status() != WL_CONNECTED) connectWifi();
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = String(SERVER_URL) + "/api/spot/" + String(spotId);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", API_KEY);

    StaticJsonDocument<200> doc;
    doc["occupied"]    = occupied;
    doc["distance_cm"] = distance;
    doc["esp32_id"]    = NODE_ID;
    String payload;
    serializeJson(doc, payload);

    int code = http.POST(payload);
    Serial.printf("[POST] place %d %s (%.1f cm) HTTP %d\n",
                  spotId, occupied ? "OCCUPEE" : "LIBRE", distance, code);
    http.end();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    // ---- Place A
    float dA = measureDistance(TRIG_A, ECHO_A);
    bool occA = (dA > 0 && dA < DISTANCE_THRESHOLD_CM);
    digitalWrite(LED_A, occA ? HIGH : LOW);
    postSpotStatus(SPOT_ID_A, occA, dA);
    delay(300);

    // ---- Place B
    float dB = measureDistance(TRIG_B, ECHO_B);
    bool occB = (dB > 0 && dB < DISTANCE_THRESHOLD_CM);
    digitalWrite(LED_B, occB ? HIGH : LOW);
    postSpotStatus(SPOT_ID_B, occB, dB);

    delay(LOOP_DELAY_MS);
}
