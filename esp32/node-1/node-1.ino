/**
 * Parking Intelligent — Nœud #1 "Panneau d'entrée"
 *
 * Configuration matérielle :
 *  - 2× HC-SR04A (places 1 et 2)
 *  - 2× LED rouge (1 par place)
 *  - 2× SBC-OLED01-V2 (SSD1306 128x64) en I2C
 *      → 2 bus I2C SÉPARÉS car les 2 OLEDs ont la même adresse 0x3C par défaut
 *      → évite de devoir re-souder un strap pour passer l'une en 0x3D
 *
 * Dépendances Arduino : WiFi, HTTPClient, ArduinoJson, Wire, Adafruit_GFX, Adafruit_SSD1306
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// ============================================================
// CONFIGURATION — À MODIFIER avant flash
// ============================================================
const char* WIFI_SSID     = "YKS_0221";
const char* WIFI_PASSWORD = "smartParking";
const char* SERVER_URL    = "http://192.168.137.1:3000"; // IP du PC sur le hotspot, sans slash final
const char* API_KEY       = "d62793cc0e30009e4bfe4d81448741264ffd95005bacfd86235944184ab2a0fe";
const char* NODE_ID       = "node-1";

const int SPOT_ID_A = 1;
const int SPOT_ID_B = 2;

// ============================================================
// CÂBLAGE GPIO — voir esp32/WIRING.md
// ============================================================
// Capteurs ultrasoniques HC-SR04A
#define TRIG_A 25
#define ECHO_A 26
#define TRIG_B 27
#define ECHO_B 14

// LEDs locales (allumées si occupé)
#define LED_A 13
#define LED_B 4

// Bus I2C #0 → OLED gauche (adresse 0x3C)
#define OLED_SDA_L 21
#define OLED_SCL_L 22

// Bus I2C #1 → OLED droite (adresse 0x3C aussi, mais sur un autre bus)
#define OLED_SDA_R 33
#define OLED_SCL_R 32

// ============================================================
// PARAMÈTRES MÉTIER
// ============================================================
const float DISTANCE_THRESHOLD_CM    = 20.0;
const unsigned long LOOP_DELAY_MS    = 4500;
const unsigned long PANEL_REFRESH_MS = 5000;

const char* NTP_SERVER       = "pool.ntp.org";
const long  GMT_OFFSET       = 3600;
const int   DAYLIGHT_OFFSET  = 3600;

// ============================================================
// OBJETS GLOBAUX
// ============================================================
TwoWire I2C_R = TwoWire(1);  // bus I2C #1 (Wire1)

Adafruit_SSD1306 oledLeft (128, 64, &Wire,  -1);
Adafruit_SSD1306 oledRight(128, 64, &I2C_R, -1);

bool spotsState[6] = {false, false, false, false, false, false};
unsigned long lastPanelRefresh = 0;

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[node-1] Boot panneau d'entree");

    pinMode(TRIG_A, OUTPUT); pinMode(ECHO_A, INPUT);
    pinMode(TRIG_B, OUTPUT); pinMode(ECHO_B, INPUT);
    pinMode(LED_A, OUTPUT);  digitalWrite(LED_A, LOW);
    pinMode(LED_B, OUTPUT);  digitalWrite(LED_B, LOW);

    Wire.begin(OLED_SDA_L, OLED_SCL_L);
    I2C_R.begin(OLED_SDA_R, OLED_SCL_R);

    if (!oledLeft.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[OLED-L] init KO (verifier SDA=21, SCL=22, alim 3.3V, adresse 0x3C)");
    }
    if (!oledRight.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[OLED-R] init KO (verifier SDA=33, SCL=32, alim 3.3V, adresse 0x3C)");
    }
    showBootScreen();

    connectWifi();
    configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
}

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

float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000);
    if (duration == 0) return -1.0;
    return (duration * 0.034) / 2.0;
}

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

bool fetchAllSpots() {
    if (WiFi.status() != WL_CONNECTED) connectWifi();
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = String(SERVER_URL) + "/api/spots";
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[GET] /api/spots HTTP %d\n", code);
        http.end();
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    if (err) {
        Serial.printf("[GET] JSON err: %s\n", err.c_str());
        return false;
    }

    for (JsonObject spot : doc.as<JsonArray>()) {
        int id = spot["id"] | 0;
        bool occ = spot["is_occupied"] | false;
        if (id >= 1 && id <= 6) spotsState[id - 1] = occ;
    }
    return true;
}

void showBootScreen() {
    oledLeft.clearDisplay();
    oledLeft.setTextColor(SSD1306_WHITE);
    oledLeft.setTextSize(2);
    oledLeft.setCursor(8, 24);
    oledLeft.println("PARKING");
    oledLeft.display();

    oledRight.clearDisplay();
    oledRight.setTextColor(SSD1306_WHITE);
    oledRight.setTextSize(2);
    oledRight.setCursor(20, 24);
    oledRight.println("BOOT...");
    oledRight.display();
}

void renderLeftPanel() {
    oledLeft.clearDisplay();
    oledLeft.setTextColor(SSD1306_WHITE);
    oledLeft.setTextSize(1);
    oledLeft.setCursor(0, 0);
    oledLeft.print("PARKING - 6 places");
    oledLeft.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    const int cols = 3;
    const int cellW = 42, cellH = 25;
    const int startX = 1, startY = 13;

    for (int i = 0; i < 6; i++) {
        int col = i % cols, row = i / cols;
        int x = startX + col * cellW;
        int y = startY + row * cellH;

        if (spotsState[i]) {
            oledLeft.fillRect(x, y, cellW - 2, cellH - 2, SSD1306_WHITE);
            oledLeft.setTextColor(SSD1306_BLACK);
        } else {
            oledLeft.drawRect(x, y, cellW - 2, cellH - 2, SSD1306_WHITE);
            oledLeft.setTextColor(SSD1306_WHITE);
        }
        oledLeft.setCursor(x + 6, y + 4);
        oledLeft.print("P");
        oledLeft.print(i + 1);
        oledLeft.setCursor(x + 4, y + 14);
        oledLeft.print(spotsState[i] ? "OCC" : "LIB");
    }
    oledLeft.display();
}

void renderRightPanel() {
    int free = 0;
    for (int i = 0; i < 6; i++) if (!spotsState[i]) free++;

    oledRight.clearDisplay();
    oledRight.setTextColor(SSD1306_WHITE);

    oledRight.setTextSize(3);
    oledRight.setCursor(20, 5);
    oledRight.print(free);
    oledRight.print("/6");

    oledRight.setTextSize(1);
    oledRight.setCursor(35, 35);
    oledRight.print("LIBRES");

    char timeStr[9];
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    } else {
        strcpy(timeStr, "--:--:--");
    }
    oledRight.setCursor(28, 52);
    oledRight.print(timeStr);
    oledRight.display();
}

void loop() {
    float dA = measureDistance(TRIG_A, ECHO_A);
    bool occA = (dA > 0 && dA < DISTANCE_THRESHOLD_CM);
    digitalWrite(LED_A, occA ? HIGH : LOW);
    postSpotStatus(SPOT_ID_A, occA, dA);
    spotsState[SPOT_ID_A - 1] = occA;
    delay(300);

    float dB = measureDistance(TRIG_B, ECHO_B);
    bool occB = (dB > 0 && dB < DISTANCE_THRESHOLD_CM);
    digitalWrite(LED_B, occB ? HIGH : LOW);
    postSpotStatus(SPOT_ID_B, occB, dB);
    spotsState[SPOT_ID_B - 1] = occB;

    if (millis() - lastPanelRefresh >= PANEL_REFRESH_MS) {
        fetchAllSpots();
        renderLeftPanel();
        renderRightPanel();
        lastPanelRefresh = millis();
    }

    delay(LOOP_DELAY_MS);
}
