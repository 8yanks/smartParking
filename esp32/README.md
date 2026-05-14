# ESP32 — Code embarqué Parking Intelligent

Code Arduino pour les 3 nœuds ESP32 du parking 6 places.

> 🛠️ **Tu montes les composants pour la première fois ?** Suis le [**guide de montage pas à pas**](WIRING.md) plutôt que ce README — il commence par "Hello World" et te fait tester chaque composant individuellement avant d'assembler.

## Architecture matérielle

| Nœud | Rôle | Capteurs | LEDs | OLEDs | Sketch |
|------|------|----------|------|-------|--------|
| **#1 "panneau"** | Capteurs places 1-2 + panneau d'information à l'entrée | 2× HC-SR04A | 2× LED rouge | **2× SBC-OLED01-V2 SPI** | `node-1.ino` |
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

### Nœuds capteurs (#2 et #3)

```
ESP32 NodeMCU                  HC-SR04A #A (place A)
─────────────                  ──────────────────────
GPIO 25  ───────────────────►  TRIG
GPIO 26  ◄───────────────────  ECHO
5V       ─────────────────────  VCC
GND      ─────────────────────  GND

                               HC-SR04A #B (place B)
                               ──────────────────────
GPIO 27  ───────────────────►  TRIG
GPIO 14  ◄───────────────────  ECHO
5V       ─────────────────────  VCC
GND      ─────────────────────  GND

                               LED rouge place A
                               ─────────────────
GPIO 32  ──[ R 220Ω ]──────►  Anode (+)
                               Cathode (−) ──► GND

                               LED rouge place B
                               ─────────────────
GPIO 33  ──[ R 220Ω ]──────►  Anode (+)
                               Cathode (−) ──► GND
```

### Nœud panneau (#1)

Idem que les nœuds capteurs (capteurs + 2 LEDs sur GPIO 25/26/27/14/32/33), **plus** les 2 OLEDs en SPI :

```
ESP32 NodeMCU                  Bus SPI partagé          OLED gauche  OLED droite
─────────────                  ───────────────          ───────────  ────────────
GPIO 18  ───────────────────►  SCK / SCL          ───►  SCK          SCK
GPIO 23  ───────────────────►  MOSI / SDA         ───►  SDA          SDA
GPIO 17  ───────────────────►  RES (reset)        ───►  RES          RES
GPIO 16  ───────────────────►  DC (data/command)  ───►  DC           DC
GPIO  5  ───────────────────►                            CS           ─
GPIO  4  ───────────────────►                            ─            CS
3.3V     ─────────────────────                           VCC          VCC
GND      ─────────────────────                           GND          GND
```

> ⚠️ Vérifier la tension d'alim des OLED — la SBC-OLED01-V2 accepte 3.3V (utiliser le rail 3V3 de l'ESP32).

### Récap broches utilisées

| GPIO | Fonction | Sur tous les nœuds |
|------|----------|--------------------|
| 25 | TRIG capteur A | ✅ |
| 26 | ECHO capteur A | ✅ |
| 27 | TRIG capteur B | ✅ |
| 14 | ECHO capteur B | ✅ |
| 32 | LED place A | ✅ |
| 33 | LED place B | ✅ |
| 18 | OLED SCK | nœud #1 uniquement |
| 23 | OLED MOSI | nœud #1 uniquement |
| 17 | OLED RES | nœud #1 uniquement |
| 16 | OLED DC | nœud #1 uniquement |
| 5  | OLED gauche CS | nœud #1 uniquement |
| 4  | OLED droite CS | nœud #1 uniquement |

Broches strap à éviter (pas utilisées ici) : 0, 2, 12, 15. Broches input-only à éviter pour LEDs/OLED : 34, 35, 36, 39.

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

## API attendue côté Symfony

| Méthode | Route | Body | Auth | Réponse |
|---------|-------|------|------|---------|
| POST | `/api/spot/{id}` | `{"occupied":bool, "distance_cm":float, "esp32_id":string}` | `X-API-Key` | `200 OK` |
| GET | `/api/spots` | — | publique | `[{"id":1, "name":"Place A1", "is_occupied":false}, ...]` |

Le nœud #1 utilise `GET /api/spots` toutes les ~5 s pour rafraîchir le panneau OLED.
