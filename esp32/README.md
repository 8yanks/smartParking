# ESP32 — Code embarqué Parking Intelligent

Code Arduino pour les 3 nœuds ESP32 du parking 6 places.

> 🛠️ **Tu montes les composants pour la première fois ?** Suis le [**guide de montage pas à pas**](WIRING.md) plutôt que ce README — il commence par "Hello World" et te fait tester chaque composant individuellement avant d'assembler.

## Architecture matérielle

| Nœud | Rôle | Capteurs | LEDs | OLEDs | Sketch |
|------|------|----------|------|-------|--------|
| **#1 "panneau"** | Capteurs places 1-2 + panneau d'information à l'entrée | 2× HC-SR04A | 2× LED rouge | **2× SBC-OLED01-V2 I2C (double-bus)** | `node-1.ino` |
| **#2** | Capteurs places 3-4 | 2× HC-SR04A | 2× LED rouge | — | `node-sensor.ino` (NODE_ID = `node-2`) |
| **#3** | Capteurs places 5-6 | 2× HC-SR04A | 2× LED rouge | — | `node-sensor.ino` (NODE_ID = `node-3`) |

Total : **3 ESP32, 6 capteurs, 6 LEDs (+1 spare), 2 OLEDs**.

## Bibliothèques Arduino requises

À installer via le gestionnaire de bibliothèques (Arduino IDE) :

| Lib | Auteur | Usage |
|-----|--------|-------|
| `WiFi` | (intégrée ESP32) | Connexion WiFi |
| `HTTPClient` | (intégrée ESP32) | POST/GET vers le serveur Symfony |
| `ArduinoJson` | Benoit Blanchon | Sérialisation JSON |
| `Adafruit GFX Library` | Adafruit | Primitives graphiques (nœud #1 uniquement) |
| `Adafruit SSD1306` | Adafruit | Pilote des OLEDs (nœud #1 uniquement) |

## Câblage GPIO

Le pinout détaillé (broche par broche, avec tableau) est dans [`WIRING.md`](WIRING.md). Récap rapide :

| GPIO | Fonction | Nœuds capteurs (#2, #3) | Nœud panneau (#1) |
|------|----------|-------------------------|-------------------|
| 25 | TRIG capteur A | ✅ | ✅ |
| 26 | ECHO capteur A | ✅ | ✅ |
| 27 | TRIG capteur B | ✅ | ✅ |
| 14 | ECHO capteur B | ✅ | ✅ |
| 32 | LED A | ✅ | — (occupé par SCL bus I2C #1) |
| 33 | LED B | ✅ | — (occupé par SDA bus I2C #1) |
| 13 | LED A | — | ✅ (déplacée à cause du bus I2C) |
| 4  | LED B | — | ✅ (déplacée à cause du bus I2C) |
| 21 / 22 | SDA / SCL bus I2C #0 (OLED gauche) | — | ✅ |
| 33 / 32 | SDA / SCL bus I2C #1 (OLED droite) | — | ✅ |

> Les 2 OLEDs partagent l'adresse I2C `0x3C` → on utilise **deux bus I2C distincts** sur l'ESP32 (Wire et Wire1) pour les distinguer sans soudure.

**Alim** : capteurs HC-SR04A sur **5V (VIN)**, OLEDs sur **3.3V** (jamais 5V — risque de griller).

Broches strap à éviter : 0, 2, 12, 15. Broches input-only : 34, 35, 36, 39.

## Calcul des résistances LED

LED rouge typique : Vf ≈ 2,0 V, If = 5–10 mA.
Source GPIO ESP32 : 3,3 V.

```
R = (3.3 - 2.0) / 0.006 ≈ 220 Ω
```

→ **220 Ω** convient (chute ~6 mA, bien dans la limite des 12 mA recommandés par GPIO ESP32).

## Configuration avant flash

Modifier les variables en haut de chaque sketch :

```cpp
const char* WIFI_SSID     = "...";
const char* WIFI_PASSWORD = "...";
const char* SERVER_URL    = "https://votre-serveur.fr"; // sans slash final
const char* API_KEY       = "..."; // doit matcher la clé X-API-Key côté Symfony
```

Pour `node-sensor.ino`, choisir aussi le bon `NODE_ID` (`node-2` ou `node-3`) en commentant/décommentant le bon bloc.

## Test rapide (sans serveur)

Le moniteur série (115200 baud) affiche pour chaque mesure :
```
[POST] place 1 LIBRE (47.3 cm) HTTP -1
```
Le `HTTP -1` est normal tant que le serveur n'est pas lancé. La détection capteur + LED locale fonctionnent quand même.

## API attendue côté Next.js

| Méthode | Route | Body | Auth | Réponse |
|---------|-------|------|------|---------|
| POST | `/api/spot/{id}` | `{"occupied":bool, "distance_cm":float, "esp32_id":string}` | `X-API-Key` | `200 OK` |
| GET | `/api/spots` | — | publique | `[{"id":1, "name":"Place A1", "is_occupied":false, ...}, ...]` |

Le nœud #1 utilise `GET /api/spots` toutes les ~5 s pour rafraîchir le panneau OLED.
