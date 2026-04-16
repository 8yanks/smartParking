#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================================================
// CONFIGURATION
// ===================================================
const char* WIFI_SSID     = "ParkingIntelligent";
const char* WIFI_PASSWORD = "parking2026";
const char* SERVER_IP     = "192.168.137.1";
const int   SERVER_PORT   = 8000;
const char* API_KEY       = "une_cle_secrete_longue_pour_esp32_changez_moi";
const char* ESP32_ID      = "node-2";

const int SPOT_ID_1 = 3;  // Place A3
const int SPOT_ID_2 = 4;  // Place B1

const int TRIG_1 = 5,  ECHO_1 = 18;
const int TRIG_2 = 19, ECHO_2 = 23;  // GPIO 21 réservé OLED SDA
const int LED_1  = 2,  LED_2  = 4;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const float THRESHOLD_CM = 20.0;

// ===================================================
// ARCHITECTURE
// ===================================================
// COUCHE 1 — AUTONOME : capteurs → LEDs (sans réseau)
// COUCHE 2 — REPORTING : HTTP → Symfony (best-effort, timeout 1s)
// RÉSEAU : Hotspot Windows 192.168.137.1
//          SSID: ParkingIntelligent | MDP: parking2026
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

void updateDisplay(bool occ1, float d1, bool occ2, float d2) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("== PARKING NOEUD 2 ==");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.printf("A3: %s", occ1 ? "OCCUPEE" : "LIBRE  ");
    if (d1 > 0) display.printf(" (%.0fcm)", d1);

    display.drawLine(0, 36, 127, 36, SSD1306_WHITE);

    display.setCursor(0, 40);
    display.printf("B1: %s", occ2 ? "OCCUPEE" : "LIBRE  ");
    if (d2 > 0) display.printf(" (%.0fcm)", d2);

    display.display();
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
    Serial.println("\n=== Parking Intelligent — node-2 ===");

    pinMode(LED_1, OUTPUT); digitalWrite(LED_1, LOW);
    pinMode(LED_2, OUTPUT); digitalWrite(LED_2, LOW);
    pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
    pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);

    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Erreur OLED");
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Démarrage...");
        display.display();
    }
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

    // 3. OLED
    updateDisplay(o1, d1, o2, d2);

    // 4. Reporting (best-effort)
    sendSpotData(SPOT_ID_1, o1, d1);
    delay(200);
    sendSpotData(SPOT_ID_2, o2, d2);

    delay(4000);
}
