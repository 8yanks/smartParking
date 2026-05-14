# Guide de montage physique — pas à pas

Ce guide t'accompagne du tout début (composants dans leur sachet) jusqu'aux 3 nœuds montés et fonctionnels. **Suis l'ordre** — chaque étape valide la précédente avant d'ajouter de la complexité.

> Pour les broches GPIO et le résumé technique, voir [`README.md`](README.md). Ce document est le mode d'emploi pratique.

---

## ⚠️ À vérifier avant de commencer

### Modules OLED SBC-OLED01-V2

Ces modules Joy-IT sont **configurables I2C ou SPI via des résistances de jumper** au dos. Par défaut ils arrivent en **I2C 4-pins**, mais notre code est en SPI **7-pins**. Tu dois soit :

- Re-souder les jumpers selon la doc Joy-IT du SBC-OLED01-V2 pour passer en mode SPI 4-fils
- Ou regarder le PCB : 7 pins exposés (VCC, GND, SCK, SDA, RES, DC, CS) = SPI ; 4 pins (VCC, GND, SCK/SCL, SDA) = I2C

Si tu n'es pas sûr, prends une photo du module (recto + verso) — je t'aide à identifier la config dans la nouvelle session.

### Sécurité électrique

- **Toujours débrancher l'USB** avant de modifier un câblage
- Les capteurs HC-SR04A se branchent en **5V** (rail VIN/5V de l'ESP32, pas 3.3V — sinon ils sous-fonctionnent)
- Les OLED se branchent en **3.3V** (pas 5V — risque de griller l'écran)
- Les LEDs **toujours avec leur résistance 220 Ω** en série (sinon court-circuit + LED grillée + GPIO endommagé)

---

## Étape 1 — Test "Hello World" sur 1 ESP32 (15 min)

**But** : valider qu'Arduino IDE flashe correctement, que l'ESP32 fonctionne, et que tu maîtrises l'upload.

### Matériel
- 1 ESP32 NodeMCU
- 1 câble micro-USB

### Manip
1. Branche l'ESP32 en USB sur le PC.
2. Dans Arduino IDE :
   - **Tools → Board → esp32 → ESP32 Dev Module**
   - **Tools → Port → COMx** (le COM qui apparaît quand tu branches)
3. Ouvre **File → Examples → 01.Basics → Blink**
4. Modifie la ligne `#define LED_BUILTIN 2` (la LED bleue intégrée est sur GPIO 2 sur la plupart des NodeMCU)
5. Clique **Upload (→)**. Pendant l'upload, **maintiens le bouton BOOT** sur l'ESP32 si nécessaire.

### Validation
La LED bleue de l'ESP32 doit clignoter à 1 Hz. Si oui : ✅ tu peux flasher. Si non : voir [troubleshooting](#troubleshooting) en bas.

---

## Étape 2 — Test 1 capteur HC-SR04A (10 min)

**But** : vérifier que tu sais lire une distance.

### Câblage sur breadboard

```
ESP32 NodeMCU                   HC-SR04A
─────────────                   ──────────
       5V (VIN)  ───────────►  VCC
       GND       ───────────►  GND
       GPIO 25   ───────────►  TRIG
       GPIO 26   ◄───────────  ECHO
```

> 💡 Le HC-SR04**A** (variante "low voltage") fonctionne aussi en 3.3V, mais pour rester safe utilise 5V (VIN) — ça donne une plage de mesure plus stable.

### Sketch de test

Crée un nouveau sketch dans Arduino IDE :

```cpp
#define TRIG 25
#define ECHO 26

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long d = pulseIn(ECHO, HIGH, 30000);
  Serial.printf("Distance: %.1f cm\n", (d * 0.034) / 2.0);
  delay(500);
}
```

### Validation
Ouvre **Tools → Serial Monitor (115200)**. Bouge ta main devant le capteur — la distance doit varier de quelques cm à 100+ cm. Si oui : ✅

---

## Étape 3 — Ajout d'1 LED rouge (5 min)

**But** : savoir piloter une LED via GPIO.

### Câblage (ajouter à l'étape 2)

```
ESP32 GPIO 32  ──[ R 220Ω ]──►|◄── ESP32 GND
                              LED
                          (anode +)→(cathode -)
```

> 💡 La patte **longue** de la LED = anode (+) → côté résistance/GPIO.
> La patte **courte** = cathode (−) → côté GND.

### Sketch (extension du précédent)

Ajoute :
```cpp
#define LED 32

// dans setup() :
pinMode(LED, OUTPUT);

// dans loop(), après le calcul de distance :
float dist = (d * 0.034) / 2.0;
digitalWrite(LED, (dist > 0 && dist < 20.0) ? HIGH : LOW);
```

### Validation
Bouge ta main à moins de 20 cm → la LED s'allume. Au-delà → s'éteint. ✅

---

## Étape 4 — Test WiFi + POST vers le serveur (15 min)

**But** : vérifier que l'ESP32 peut envoyer une mesure au serveur Next.js.

**Prérequis** :
- Serveur Next.js qui tourne (`npm run dev -- -H 0.0.0.0` sur ton portable)
- Hotspot WiFi actif sur le portable
- IP du portable connue (ex: `192.168.137.1`)
- Clé `ESP32_API_KEY` du `.env` notée

### Manip

Ouvre `esp32/node-sensor.ino` dans Arduino IDE et modifie la section CONFIG :
```cpp
const char* WIFI_SSID     = "YKS_0221";        // ton SSID
const char* WIFI_PASSWORD = "smartParking";     // ton password
const char* SERVER_URL    = "http://192.168.137.1:3000";  // IP du portable
const char* API_KEY       = "...";              // valeur de ESP32_API_KEY
const char* NODE_ID       = "node-2";
const int SPOT_ID_A       = 3;
const int SPOT_ID_B       = 4;
```

Garde le câblage de l'étape 3 (1 capteur + 1 LED), branche-le sur les broches du capteur A (TRIG=25, ECHO=26, LED=32). Le capteur B et LED B ne sont pas encore câblés — tu verras `HTTP -1` ou un timeout pour la place 4, c'est normal.

Flash et ouvre le moniteur série. Tu dois voir :
```
[node-2] Boot capteur
[WiFi] connexion....
[WiFi] OK 192.168.137.X
[POST] place 3 LIBRE (47.3 cm) HTTP 200
```

`HTTP 200` = ✅ le serveur a accepté la mesure.

Vérifie côté serveur : `curl http://localhost:3000/api/spots` doit montrer la place 3 mise à jour avec `last_updated_at` récent.

---

## Étape 5 — Compléter un nœud capteur entier (20 min)

**But** : avoir un nœud avec ses 2 capteurs + 2 LEDs prêt pour la place finale.

### Câblage final d'un nœud capteur (#2 ou #3)

```
                  ESP32 NodeMCU
                  ┌─────────────┐
              5V ─┤ VIN         ├─ 3.3V (non utilisé ici)
                  │             │
            GND ──┤ GND     GND ├──── GND breadboard rail
                  │             │
   capteur A ────┤ 25 (TRIG)    │
   capteur A ────┤ 26 (ECHO)    │
   capteur B ────┤ 27 (TRIG)    │
   capteur B ────┤ 14 (ECHO)    │
                  │             │
       LED A ────┤ 32 (output)  │
       LED B ────┤ 33 (output)  │
                  └─────────────┘
                       │ USB
                       ▼ alimentation
```

Disposition recommandée sur breadboard :
- Rails d'alim haut/bas : 5V (rouge) et GND (noir)
- ESP32 au centre, broches accessibles des 2 côtés
- 2 capteurs HC-SR04A à gauche
- 2 LEDs + résistances à droite

### Validation
Au moniteur série, tu dois voir alternativement :
```
[POST] place 3 LIBRE (47.3 cm) HTTP 200
[POST] place 4 OCCUPEE (8.2 cm) HTTP 200
```

Et `curl http://localhost:3000/api/spots` doit confirmer les 2 places mises à jour.

---

## Étape 6 — Dupliquer pour les 3 nœuds capteurs (30 min)

Refais l'étape 5 sur 2 autres breadboards :

| ESP32 | Sketch | NODE_ID | SPOT_ID_A | SPOT_ID_B |
|-------|--------|---------|-----------|-----------|
| #2 | `node-sensor.ino` | `node-2` | 3 | 4 |
| #3 | `node-sensor.ino` | `node-3` | 5 | 6 |

> Le nœud #1 (panneau) attend l'étape 7 — il a un câblage différent.

À ce stade, tu as **4 places (3, 4, 5, 6) qui remontent leur état au serveur** ; les places 1 et 2 viendront avec le nœud panneau.

---

## Étape 7 — Câblage du nœud panneau (#1) avec les 2 OLEDs (45 min)

C'est le plus complexe — prends ton temps. **Vérifie d'abord** que tes OLEDs sont bien configurées en mode SPI (voir l'avertissement en haut de ce document).

### Câblage capteurs + LEDs (identique aux nœuds capteurs)

| ESP32 | Capteur A (place 1) | Capteur B (place 2) |
|-------|---------------------|---------------------|
| 5V | VCC | VCC |
| GND | GND | GND |
| GPIO 25 | TRIG | — |
| GPIO 26 | ECHO | — |
| GPIO 27 | — | TRIG |
| GPIO 14 | — | ECHO |

| ESP32 | LED A (place 1) | LED B (place 2) |
|-------|-----------------|-----------------|
| GPIO 32 | → 220Ω → anode | — |
| GPIO 33 | — | → 220Ω → anode |
| GND | cathode | cathode |

### Câblage OLEDs SPI (la nouveauté)

**SPI partagé** : SCK, MOSI, RES, DC sont communs aux 2 écrans. Seul le **CS** est différent pour chacun.

```
ESP32 NodeMCU             OLED gauche       OLED droite
─────────────             ───────────       ───────────
3.3V       ─────────────► VCC ────────────► VCC
GND        ─────────────► GND ────────────► GND
GPIO 18 (SCK)  ─────────► SCK ────────────► SCK
GPIO 23 (MOSI) ─────────► SDA ────────────► SDA
GPIO 17 (RES)  ─────────► RES ────────────► RES
GPIO 16 (DC)   ─────────► DC  ────────────► DC
GPIO  5 (CS-L) ─────────► CS                ─
GPIO  4 (CS-R) ──────────────────────────► CS
```

**Conseil pratique** : utilise une **seconde breadboard** dédiée aux 2 OLEDs (côte à côte), reliée à la première par des jumpers male-male pour les 7 lignes communes (3.3V, GND, SCK, MOSI, RES, DC, et 2× CS).

### Configuration du sketch node-1.ino

Mêmes variables que les autres (WIFI, SERVER_URL, API_KEY) plus :
```cpp
const char* NODE_ID  = "node-1";
const int SPOT_ID_A  = 1;
const int SPOT_ID_B  = 2;
```

### Flash et validation

Au boot, tu dois voir sur les écrans :
- OLED gauche : "PARKING" en gros, puis (après ~5s) la grille des 6 places (LIB / OCC)
- OLED droite : "BOOT...", puis "X/6 LIBRES" + heure

Au moniteur série :
```
[node-1] Boot panneau d'entree
[OLED-L] init OK   (ou KO si SPI mal câblé)
[OLED-R] init OK
[WiFi] OK 192.168.137.X
[POST] place 1 LIBRE (...) HTTP 200
[POST] place 2 LIBRE (...) HTTP 200
[GET] /api/spots HTTP 200
```

Si une OLED dit `init KO` : vérifie le CS correspondant (le seul fil qui les distingue).
Si les deux disent KO : vérifie les fils SCK/MOSI/DC/RES (lignes communes), et que VCC est bien sur 3.3V (pas 5V !).

---

## Étape 8 — Mise en place finale

Une fois les 3 nœuds testés sur breadboard :

1. **Position physique** :
   - Nœud #1 : à l'entrée du parking (où les 2 OLEDs sont visibles)
   - Nœuds #2 et #3 : près de leurs places respectives (3-4 et 5-6)
2. **Alimentation** : chaque ESP32 sur USB 5V (alim murale ou batterie). Distance USB ≤ 1,5 m pour éviter chute de tension.
3. **Vérification finale** : tous les 3 nœuds postent à `/api/spots` toutes les ~5 s. Le panneau OLED se met à jour.

---

## Troubleshooting

### L'ESP32 n'apparaît pas dans `Tools → Port`

- Driver USB manquant. Pour les NodeMCU récents : driver **CP210x** (Silicon Labs). Pour les plus anciens : **CH340** (WCH).
  - CP210x : <https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers>
  - CH340 : <https://sparks.gogo.co.nz/ch340.html>

### Upload échoue avec "Failed to connect... Timed out waiting for packet header"

- Maintiens le bouton **BOOT** de l'ESP32 enfoncé pendant que tu cliques Upload, jusqu'à ce que la barre de progression démarre.
- Réduis la baud rate : Tools → Upload Speed → 115200.

### `HTTP -1` ou `[WiFi] echec`

- Vérifie SSID/password (sensibles à la casse).
- Vérifie que l'ESP32 est dans la portée du hotspot (< 5 m pour être safe).
- Vérifie que le hotspot est en **2.4 GHz** (pas 5 GHz).
- Vérifie que le firewall Windows autorise les connexions entrantes sur le port 3000.
- Test depuis un autre device sur le hotspot : `curl http://192.168.137.1:3000/api/spots` doit fonctionner.

### Capteur HC-SR04A retourne toujours -1.0 ou 0

- Vérifie qu'il est bien sur **5V** (rail VIN), pas 3.3V.
- Inverse TRIG et ECHO pour vérifier qu'ils ne sont pas croisés.
- Le capteur peut être défectueux — teste avec un autre.

### OLED blanche / noire / pas d'affichage

- Vérifie que le module est en mode **SPI** (et pas I2C par défaut).
- Vérifie que VCC est bien sur **3.3V** (pas 5V — risque de griller).
- Inverse les CS si une seule OLED s'affiche.
- Lance d'abord le sketch **example** d'Adafruit_SSD1306 pour valider l'OLED isolément avant de tout intégrer.
