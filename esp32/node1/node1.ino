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
const char* ESP32_ID      = "node-1";

// Places gérées par ce nœud
const int SPOT_ID_1 = 1;  // Place A1
const int SPOT_ID_2 = 2;  // Place A2

// Broches capteur 1 (place A1)
const int TRIG_1 = 5;
const int ECHO_1 = 18;

// Broches capteur 2 (place A2)
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
    display.println("== PARKING NOEUD 1 ==");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Place 1
    display.setCursor(0, 16);
    display.printf("Place A1 (ID %d):", SPOT_ID_1);
    display.setCursor(0, 26);
    display.setTextSize(2);
    display.println(occ1 ? "OCCUPEE" : "LIBRE");
    display.setTextSize(1);

    // Séparateur
    display.drawLine(0, 42, 127, 42, SSD1306_WHITE);

    // Place 2
    display.setCursor(0, 46);
    display.printf("Place A2 (ID %d):", SPOT_ID_2);
    display.setCursor(0, 56);
    display.println(occ2 ? "OCCUPEE" : "LIBRE  ");

    display.display();
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
    Serial.println("\n=== Parking Intelligent — node-1 ===");

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
    // Mesure place 1
    float dist1 = measureDistance(TRIG_1, ECHO_1);
    bool occ1   = (dist1 > 0 && dist1 < THRESHOLD_CM);
    digitalWrite(LED_1, occ1 ? HIGH : LOW);
    sendSpotData(SPOT_ID_1, occ1, dist1);
    delay(500);

    // Mesure place 2
    float dist2 = measureDistance(TRIG_2, ECHO_2);
    bool occ2   = (dist2 > 0 && dist2 < THRESHOLD_CM);
    digitalWrite(LED_2, occ2 ? HIGH : LOW);
    sendSpotData(SPOT_ID_2, occ2, dist2);
    delay(500);

    // Mise à jour affichage OLED
    updateDisplay(occ1, occ2);

    delay(4000);
}
