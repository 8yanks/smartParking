# Guide de montage physique — pas à pas

> ⚙️ **Config OLED choisie : I2C** (pas de soudure à faire). Les SBC-OLED01-V2 sortent en I2C par défaut, on les utilise tels quels.

---

## ⚠️ À vérifier avant de commencer

- **Capteurs HC-SR04A → 5V** (rail VIN de l'ESP32, pas 3.3V)
- **OLEDs SBC-OLED01-V2 → 3.3V** (3V3 de l'ESP32, jamais 5V — risque de griller)
- **Servomoteur (SG90 / MG90S) → 5V** (sur VIN, jamais 3V3 — pas assez de courant)
- **LEDs toujours avec leur résistance 220 Ω** en série
- **Toujours débrancher l'USB** avant de modifier un câblage

## Topologie (matériel actuel : 5 capteurs + 1 servo)

| Nœud | Rôle | Capteurs | LEDs | OLEDs | Servo |
|------|------|----------|------|-------|-------|
| **#1 panneau / entrée** | Capteurs places 1-2 + panneau d'info + **barrière** | 2 (places 1, 2) | 2 | 2 (I2C) | **1 (barrière)** |
| **#2** | Capteurs places 3-4 | 2 (places 3, 4) | 2 | — | — |
| **#3** | Capteur **place handicap** (place 5, extrémité droite) | **1 (place 5 uniquement)** | 1 | — | — |

> 📝 Le matériel ne comporte que 5 capteurs (avant : 6). La place 5 est marquée **handicap** côté serveur (badge ♿ sur le dashboard, anneau bleu autour de la case).

---

## Pinout de référence

### Nœud panneau (#1) — 2 capteurs + 2 LEDs + 2 OLEDs I2C + **servo barrière**

| GPIO | Fonction | Branché à |
|------|----------|-----------|
| 5V (VIN) | Alim capteurs + **servo** | VCC des 2 HC-SR04A + fil rouge servo |
| 3V3 | Alim OLEDs | VCC des 2 OLEDs |
| GND | Masse commune | GND de tout (capteurs, LEDs, OLEDs, **servo fil marron/noir**) |
| 25 | TRIG capteur A (place 1) | TRIG HC-SR04A #A |
| 26 | ECHO capteur A | ECHO HC-SR04A #A |
| 27 | TRIG capteur B (place 2) | TRIG HC-SR04A #B |
| 14 | ECHO capteur B | ECHO HC-SR04A #B |
| 13 | LED A (place 1) | → 220Ω → anode LED |
| 4 | LED B (place 2) | → 220Ω → anode LED |
| 21 | SDA bus I2C #0 | SDA OLED gauche |
| 22 | SCL bus I2C #0 | SCL OLED gauche |
| 33 | SDA bus I2C #1 | SDA OLED droite |
| 32 | SCL bus I2C #1 | SCL OLED droite |
| **18** | **PWM servo barrière** | **fil jaune/orange du servo** |

> 💡 Les 2 OLEDs ont la même adresse 0x3C par défaut → c'est pourquoi on utilise **2 bus I2C séparés** sur l'ESP32 (qui en a 2 hardware).

> ⚠️ **Servo SG90 / MG90S** : 3 fils (rouge = VCC 5V, marron/noir = GND, jaune/orange = signal PWM). Le servo tire des pointes de 250-500 mA au démarrage. Si l'ESP32 reboot quand tu actives la barrière, alimente le servo via une source externe 5V (powerbank) ET relie son GND à celui de l'ESP32.

### Nœud capteur #2 — 2 capteurs + 2 LEDs

| GPIO | Fonction | Branché à |
|------|----------|-----------|
| 5V (VIN) | Alim capteurs | VCC des 2 HC-SR04A |
| GND | Masse | GND de tout |
| 25 | TRIG capteur A (place 3) | TRIG HC-SR04A #A |
| 26 | ECHO capteur A | ECHO HC-SR04A #A |
| 27 | TRIG capteur B (place 4) | TRIG HC-SR04A #B |
| 14 | ECHO capteur B | ECHO HC-SR04A #B |
| 32 | LED A (place 3) | → 220Ω → anode LED |
| 33 | LED B (place 4) | → 220Ω → anode LED |

### Nœud capteur #3 — **1 seul** capteur + 1 LED (place handicap)

| GPIO | Fonction | Branché à |
|------|----------|-----------|
| 5V (VIN) | Alim capteur | VCC HC-SR04A |
| GND | Masse | GND de tout |
| 25 | TRIG capteur A (place 5) | TRIG HC-SR04A |
| 26 | ECHO capteur A | ECHO HC-SR04A |
| 32 | LED A (place 5) | → 220Ω → anode LED |

(Pas de capteur B — c'est l'ancienne place 6, supprimée du matériel. Dans `node-sensor.ino` mettre `SPOT_ID_B = -1` pour désactiver la lecture/POST côté B.)

### Nœud panneau (#1) — pourquoi pas 32/33 pour les LEDs ?

Sur le panneau, **32 et 33 sont occupés par le bus I2C #1** (OLED droite). Du coup les LEDs vont sur 13 (A) et 4 (B). Le GPIO 18 est libre → c'est lui qui pilote le servo.

---

## 8 étapes de montage (ordre conseillé)

### 1. Hello World (15 min)

- Branche 1 ESP32 en USB
- Arduino IDE → Tools → Board → ESP32 Dev Module
- File → Examples → 01.Basics → Blink
- Modifie `LED_BUILTIN` à `2`
- Upload (maintenir BOOT pendant l'upload si l'ESP32 ne flash pas)
- ✅ La LED bleue clignote = OK

### 2. Test 1 capteur HC-SR04A (10 min)

Câblage : VCC→5V, GND→GND, TRIG→GPIO 25, ECHO→GPIO 26.

```cpp
#define TRIG 25
#define ECHO 26
void setup() { Serial.begin(115200); pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); }
void loop() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long d = pulseIn(ECHO, HIGH, 30000);
  Serial.printf("%.1f cm\n", (d * 0.034) / 2.0);
  delay(500);
}
```

Ouvre le moniteur série (115200), bouge la main → la distance varie. ✅

### 3. Test 1 LED (5 min)

Câblage : GPIO 13 → 220Ω → anode LED → cathode → GND. (Anode = patte longue.)

Ajoute au sketch précédent :
```cpp
#define LED 13
// dans setup() : pinMode(LED, OUTPUT);
// dans loop(), après le calcul :
float dist = (d * 0.034) / 2.0;
digitalWrite(LED, (dist > 0 && dist < 20.0) ? HIGH : LOW);
```

Approche la main < 20 cm → LED s'allume. ✅

### 4. WiFi + POST vers le serveur (15 min)

**Prérequis serveur** : Next.js doit tourner avec `npm run dev -- -H 0.0.0.0` sur ton portable, hotspot WiFi actif, IP du portable connue.

Ouvre `esp32/node-sensor.ino`, modifie en haut :
```cpp
const char* WIFI_SSID     = "YKS_0221";
const char* WIFI_PASSWORD = "smartParking";
const char* SERVER_URL    = "http://192.168.137.1:3000"; // IP du portable
const char* API_KEY       = "...";  // valeur de ESP32_API_KEY du .env
const char* NODE_ID       = "node-2";
const int SPOT_ID_A       = 3;
const int SPOT_ID_B       = 4;
```

Garde le câblage de l'étape 3 (capteur sur 25/26, LED sur 32 ou 13). Capteur B et LED B pas branchés → tu verras `place 4 ... -1 cm HTTP -1` en plus, normal.

Au moniteur série :
```
[node-2] Boot capteur
[WiFi] OK 192.168.137.X
[POST] place 3 LIBRE (47.3 cm) HTTP 200
```

`HTTP 200` = ✅. Vérifie côté portable : `curl http://localhost:3000/api/spots` doit montrer la place 3 mise à jour.

### 5. Compléter 1 nœud capteur (20 min)

Ajoute le 2ème capteur + 2ème LED au breadboard, selon le pinout du tableau "Nœuds capteurs" plus haut. Re-flash, vérifie les 2 places remontent.

### 6. Faire les nœuds capteurs #2 et #3 (30 min)

| ESP32 | Sketch | NODE_ID | Places | Note |
|-------|--------|---------|--------|------|
| #2 | `node-sensor.ino` | `node-2` | 3, 4 | 2 capteurs |
| #3 | `node-sensor.ino` | `node-3` | **5 uniquement** | 1 seul capteur — laisser `SPOT_ID_B = -1` |

À la fin : 3 places (3-5) remontent au serveur. Places 1-2 attendent l'étape 7.

### 7. Câblage du nœud panneau #1 avec 2 OLEDs I2C (45 min)

**Capteurs et LEDs** : suis le pinout du tableau "Nœud panneau" plus haut (LEDs sur 13 et 4, pas 32/33 !).

**OLEDs (la nouveauté)** : 4 fils chacune, sur 2 bus I2C distincts.

```
ESP32 NodeMCU                OLED gauche      OLED droite
─────────────                ───────────      ───────────
3.3V       ───────────────►  VCC ──────────►  VCC
GND        ───────────────►  GND ──────────►  GND
GPIO 21 (SDA0) ──────────►  SDA              ─
GPIO 22 (SCL0) ──────────►  SCL              ─
GPIO 33 (SDA1) ──────────────────────────►  SDA
GPIO 32 (SCL1) ──────────────────────────►  SCL
```

Flash `esp32/node-1.ino` (mêmes WIFI/SERVER_URL/API_KEY que les autres). Au moniteur série :
```
[node-1] Boot panneau
[OLED-L] init OK     (sinon vérifier SDA=21, SCL=22, alim 3.3V)
[OLED-R] init OK     (sinon vérifier SDA=33, SCL=32, alim 3.3V)
[WiFi] OK ...
[POST] place 1 ... HTTP 200
```

Affichage attendu :
- OLED gauche : "PARKING" puis (~5s) la grille des **5 cases** LIB / OCC
- OLED droite : "BOOT..." puis "X/5 LIBRES" + heure NTP

### 7bis. Câblage et test du servomoteur barrière (15 min)

**Câblage** (3 fils du servo SG90/MG90S vers l'ESP32 #1) :

```
Servo SG90              ESP32 NodeMCU
──────────              ─────────────
Fil rouge       ───►   VIN (5V)
Fil marron/noir ───►   GND
Fil jaune/orange ──►   GPIO 18
```

**Test isolé** (sans serveur, juste pour vérifier que le servo tourne) — sketch minimal :

```cpp
#include <ESP32Servo.h>
Servo barrier;
void setup() { barrier.attach(18); }
void loop() {
  barrier.write(0);   delay(2000);   // barrière fermée
  barrier.write(90);  delay(2000);   // barrière ouverte
}
```

Lib à installer : **ESP32Servo** par Kevin Harrington (gestionnaire de bibliothèques).

**Test intégré** : flash `node-1.ino` normalement. Au boot, le servo va à 0° (fermé). Ensuite, depuis le dashboard → bouton **OUVRIR** → tu devrais voir le servo bouger sous 2-3 secondes (le node-1 poll `/api/barrier` toutes les 2s).

> ⚠️ Si l'ESP32 reboote dès que tu actives le servo, c'est un problème de courant. Solution : alimenter le servo via une source 5V externe (powerbank, alim murale 5V) — connecte juste son **fil rouge à la source externe** et **garde son GND relié à celui de l'ESP32** (masse commune obligatoire pour que le signal PWM soit interprété).

### 8. Mise en place finale

Position physique :
- Nœud #1 → entrée du parking (OLEDs visibles)
- Nœuds #2, #3 → près de leurs places
- Chaque ESP32 sur USB 5V (alim murale ou batterie)

---

## Troubleshooting

### `Tools → Port` ne montre pas l'ESP32
- Driver USB manquant : **CP210x** ([silabs.com](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)) ou **CH340** ([sparks.gogo.co.nz/ch340.html](https://sparks.gogo.co.nz/ch340.html)) selon ta board.

### Upload échoue "Failed to connect"
- Maintiens le bouton **BOOT** pendant l'upload. Réduis Upload Speed à 115200.

### `HTTP -1` ou `[WiFi] echec`
- Vérifie SSID/password (sensibles à la casse).
- Hotspot bien en **2.4 GHz** (pas 5 GHz).
- Firewall Windows autorise le port 3000 (`New-NetFirewallRule -DisplayName "Next.js" -Direction Inbound -LocalPort 3000 -Protocol TCP -Action Allow` en PowerShell admin).
- Test depuis un autre device : `curl http://192.168.137.1:3000/api/spots`.

### Capteur HC-SR04A renvoie -1
- Vérifie qu'il est sur **5V** (pas 3.3V).
- Inverse TRIG/ECHO si croisés.

### OLED `init KO`
- Vérifie alim **3.3V** (pas 5V !).
- Vérifie SDA/SCL (pas inversés).
- L'adresse doit être 0x3C par défaut. Si rien ne marche, scanne le bus avec un I2C scanner pour confirmer.

### Une seule des 2 OLEDs marche
- Vérifie le 2ème bus (GPIO 33/32 pour la droite).
- Les fils SDA/SCL sont bien dédiés au bon OLED, pas crossés ?

### Le servo ne bouge pas
- Vérifie la lib **ESP32Servo** installée (pas la Servo de base).
- Mesure ~5V sur le fil rouge du servo (pas 3.3V).
- Le **GND du servo** doit être **commun avec celui de l'ESP32** sinon le signal PWM est flottant.
- Côté serveur : `curl http://localhost:3000/api/barrier` doit renvoyer `{"is_open":true|false,...}`. Si OK et le servo ne bouge pas, c'est un souci câblage/alim.

### L'ESP32 reboote dès que la barrière s'ouvre
- Pointe de courant trop forte pour le régulateur de la board. Alimente le servo via une source 5V externe (powerbank, alim USB), en gardant GND commun.
