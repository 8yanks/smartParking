# Code ESP32 — Parking Intelligent

## Matériel nécessaire
- 3x Carte NodeMCU ESP32
- 6x Capteur ultrason HC-SR04A
- Jumpers, résistances, breadboard

## Avant de flasher

1. Ouvrir le fichier `.ino` du nœud voulu dans Arduino IDE
2. Installer les bibliothèques nécessaires (Outils → Gérer les bibliothèques) :
   - **ArduinoJson** par Benoit Blanchon (v6.x)
   - **HTTPClient** (inclus avec le support ESP32)
3. Ajouter le support ESP32 dans Arduino IDE :
   - Fichier → Préférences → URL de gestionnaire de cartes supplémentaires :
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Outils → Type de carte → Gestionnaire de cartes → chercher "esp32" → Installer
4. Sélectionner la carte : Outils → Type de carte → ESP32 → **NodeMCU-32S** ou **ESP32 Dev Module**

## Configuration (dans chaque .ino)

Modifier ces lignes en haut du fichier :
```cpp
const char* WIFI_SSID     = "NOM_DE_VOTRE_WIFI";
const char* WIFI_PASSWORD = "MOT_DE_PASSE_WIFI";
const char* SERVER_IP     = "192.168.1.100"; // IP du PC (voir ci-dessous)
```

## Trouver l'IP du serveur

Sur le PC qui fait tourner Symfony, ouvrir un terminal et taper :
- Windows : `ipconfig` → chercher "Adresse IPv4" sous votre connexion WiFi
- L'IP ressemble à `192.168.1.X` ou `192.168.0.X`

## Lancer le serveur Symfony

```bash
cd C:/Users/yanis/Documents/Cours/Projet/siteReservation
symfony server:start --no-tls
```

Le serveur écoute sur le port 8000. L'ESP32 envoie ses données à :
`http://192.168.1.X:8000/api/sensor/data`

## Correspondance nœuds ↔ places

| Fichier | ESP32 ID | Place 1 | Place 2 |
|---------|----------|---------|---------|
| node1/node1.ino | node-1 | Place A1 (ID 1) | Place A2 (ID 2) |
| node2/node2.ino | node-2 | Place A3 (ID 3) | Place B1 (ID 4) |
| node3/node3.ino | node-3 | Place B2 (ID 5) | Place B3 (ID 6) |

## Test sans ESP32 (simulation via curl)

```bash
curl -X POST http://127.0.0.1:8000/api/sensor/data \
  -H "Content-Type: application/json" \
  -H "X-API-Key: une_cle_secrete_longue_pour_esp32_changez_moi" \
  -d "{\"spotId\": 1, \"distanceCm\": 8.5, \"isOccupied\": true, \"esp32Id\": \"node-1\"}"
```
