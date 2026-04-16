# Plan de construction — Circuit Parking Intelligent

**Équipe 154** — Michaux Yanis, Perret Théo, Bruneau Adam, Malespine James

---

## Vue d'ensemble

Le système est divisé en **3 nœuds ESP32 identiques**, chacun gérant **2 places de parking** :

```
┌──────────────────────────────────────────────────────────┐
│                     RÉSEAU WiFi LOCAL                    │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │   ESP32 #1   │  │   ESP32 #2   │  │   ESP32 #3   │  │
│  │ Places 1 & 2 │  │ Places 3 & 4 │  │ Places 5 & 6 │  │
│  │  + OLED      │  │  + OLED      │  │              │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│         │                 │                  │           │
│    USB-C (PC)        USB-C (PC)         USB-C (PC)      │
└──────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  PC Symfony      │
                    │  192.168.x.x     │
                    │  :8000           │
                    └──────────────────┘
```

---

## Composants utilisés

| Composant | Qté totale | Par nœud |
|-----------|-----------|---------|
| NodeMCU ESP32 | 3 | 1 |
| Capteur HC-SR04A | 6 | 2 |
| LED rouge 10mm | 6 | 2 |
| Résistance 220Ω | 6 | 2 |
| Afficheur OLED 0,96" I2C | 2 | 1 (nœuds 1 & 2 seulement) |
| Câbles jumpers M/M et F/F | selon besoin | ~15 par nœud |

> **Note alimentation :** Chaque ESP32 est alimenté via USB-C (ordinateur portable ou chargeur téléphone). Pas besoin d'alimentation externe supplémentaire — l'ESP32 fournit le 5V et 3.3V à tous les composants via ses broches.

---

## Attribution des broches GPIO

### Schéma commun aux 3 nœuds

```
ESP32 NodeMCU
    ┌──────────────────────────────┐
    │  3V3 ●──────────── OLED VCC  │
    │  GND ●──────────── OLED GND  │
    │  G21 ●──────────── OLED SDA  │  ← I2C Data  (nœuds 1 & 2 seulement)
    │  G22 ●──────────── OLED SCL  │  ← I2C Clock (nœuds 1 & 2 seulement)
    │                              │
    │  VIN ●──────────── HC-SR04A #1 VCC  │  ← 5V depuis USB
    │  VIN ●──────────── HC-SR04A #2 VCC  │
    │  GND ●──────────── HC-SR04A #1 GND  │
    │  GND ●──────────── HC-SR04A #2 GND  │
    │  G05 ●──────────── HC-SR04A #1 TRIG │
    │  G18 ●──────────── HC-SR04A #1 ECHO │
    │  G19 ●──────────── HC-SR04A #2 TRIG │
    │  G23 ●──────────── HC-SR04A #2 ECHO │  ← G21 pour nœud 3 (pas d'OLED)
    │                              │
    │  G02 ●──[220Ω]──── LED #1 (+) │
    │  G04 ●──[220Ω]──── LED #2 (+) │
    │  GND ●──────────── LED #1 (-) │
    │  GND ●──────────── LED #2 (-) │
    └──────────────────────────────┘
```

---

## Tableau complet des connexions

### Nœud 1 (Place A1 = ID 1, Place A2 = ID 2)

| Composant | Broche composant | Broche ESP32 | Fil |
|-----------|-----------------|--------------|-----|
| HC-SR04A #1 | VCC | VIN (5V) | Rouge |
| HC-SR04A #1 | GND | GND | Noir |
| HC-SR04A #1 | TRIG | GPIO 5 | Jaune |
| HC-SR04A #1 | ECHO | GPIO 18 | Vert |
| HC-SR04A #2 | VCC | VIN (5V) | Rouge |
| HC-SR04A #2 | GND | GND | Noir |
| HC-SR04A #2 | TRIG | GPIO 19 | Jaune |
| HC-SR04A #2 | ECHO | GPIO 23 | Vert |
| LED rouge #1 | Anode (+, longue patte) | GPIO 2 → [220Ω] | Orange |
| LED rouge #1 | Cathode (-, courte patte) | GND | Noir |
| LED rouge #2 | Anode (+, longue patte) | GPIO 4 → [220Ω] | Orange |
| LED rouge #2 | Cathode (-, courte patte) | GND | Noir |
| OLED | VCC | 3V3 | Rouge |
| OLED | GND | GND | Noir |
| OLED | SDA | GPIO 21 | Bleu |
| OLED | SCL | GPIO 22 | Violet |

### Nœud 2 (Place A3 = ID 3, Place B1 = ID 4)
**Connexions identiques au Nœud 1.**

### Nœud 3 (Place B2 = ID 5, Place B3 = ID 6)
**Connexions identiques au Nœud 1, SAUF :**
- Pas d'OLED (GPIO 21 et 22 libres)
- HC-SR04A #2 ECHO → **GPIO 21** (au lieu de 23)

---

## Comment connecter les composants (pas à pas)

### Matériel recommandé avant de commencer
- 1 ESP32 NodeMCU
- 2 capteurs HC-SR04A
- 2 LEDs rouges 10mm
- 2 résistances 220Ω (bandes couleur : rouge-rouge-marron-or)
- Câbles jumpers M/M (mâle-mâle) et F/F (femelle-femelle)
- Optionnel : petite breadboard (plaque d'essai blanche) pour maintenir les composants

### Étape 1 — Alimenter les capteurs HC-SR04A
1. Prenez 2 câbles **rouges** M/M
2. Branchez-les sur la broche **VIN** de l'ESP32 (broche 5V)
3. L'autre extrémité va sur la broche **VCC** de chaque capteur

4. Prenez 2 câbles **noirs** M/M
5. Branchez-les sur une broche **GND** de l'ESP32
6. L'autre extrémité va sur la broche **GND** de chaque capteur

### Étape 2 — Brancher les signaux TRIG et ECHO
Capteur 1 (place 1) :
- Câble **jaune** : GPIO 5 → TRIG capteur 1
- Câble **vert** : GPIO 18 → ECHO capteur 1

Capteur 2 (place 2) :
- Câble **jaune** : GPIO 19 → TRIG capteur 2
- Câble **vert** : GPIO 23 → ECHO capteur 2 *(GPIO 21 pour nœud 3)*

> **Pourquoi le HC-SR04A ne nécessite pas de diviseur de tension ?**
> Le modèle HC-SR04**A** (avec le "A") est la version 3.3V, compatible directement avec l'ESP32. Le modèle sans "A" nécessiterait 2 résistances supplémentaires. Vérifiez l'étiquette sur votre capteur.

### Étape 3 — Brancher les LEDs avec résistances

> **Pourquoi une résistance ?** Sans résistance, trop de courant traverse la LED et elle grille immédiatement. La résistance 220Ω limite le courant à ~6mA, ce qui est parfait.

1. Identifiez la **longue patte** (anode, +) et la **courte patte** (cathode, -)
2. Insérez la résistance 220Ω en série avec la longue patte :
   - Résistance entre GPIO 2 et longue patte LED #1
   - Résistance entre GPIO 4 et longue patte LED #2
3. Reliez les **courtes pattes** au GND de l'ESP32

```
GPIO 2 ──────[220Ω]──────>|── GND
                          LED
                        (longue)  (courte)
```

> **Astuce breadboard :** Si vous avez une plaque d'essai, insérez la résistance et la LED dans la même colonne de trous. C'est plus propre et plus solide.

### Étape 4 — Brancher l'OLED (nœuds 1 et 2 uniquement)

L'OLED communique en **I2C** (seulement 2 fils de données) :
1. VCC → **3V3** de l'ESP32 (pas VIN/5V ! L'OLED est en 3.3V)
2. GND → **GND**
3. SDA → **GPIO 21**
4. SCL → **GPIO 22**

> **Attention :** Sur certains modules OLED, les broches SDA et SCL peuvent être inversées selon le fabricant. Si l'écran ne s'affiche pas, essayez d'inverser SDA et SCL.

---

## Identifier les résistances 220Ω

Dans votre assortiment, une résistance 220Ω a ces bandes de couleur :

```
│ Rouge │ Rouge │ Marron │ Or │
│   2   │   2   │  ×10   │ 5% │
= 22 × 10 = 220Ω
```

---

## Schéma de câblage visuel (nœud complet)

```
                    ┌──────────────────────────────────┐
                    │         ESP32 NodeMCU             │
              USB-C │                                   │
              ──────┤ 5V (VIN) ─────────────────────┐  │
                    │                               │  │
                    │ 3V3 ──── OLED VCC             │  │
                    │ GND ──── OLED GND             │  │
                    │ G21 ──── OLED SDA             │  │
                    │ G22 ──── OLED SCL             │  │
                    │                               │  │
                    │ G05 ──── TRIG ┐               │  │
                    │ G18 ──── ECHO ┤ HC-SR04A #1 ←─┘  │
                    │         GND ──┤               │  │
                    │         VCC ──┘               │  │
                    │                               │  │
                    │ G19 ──── TRIG ┐               │  │
                    │ G23 ──── ECHO ┤ HC-SR04A #2 ←─┘  │
                    │         GND ──┤                  │
                    │         VCC ──┘                  │
                    │                                  │
                    │ G02 ──[220Ω]──>|── GND  (LED #1) │
                    │ G04 ──[220Ω]──>|── GND  (LED #2) │
                    │                                  │
                    └──────────────────────────────────┘
```

---

## Code Arduino à mettre à jour

> ⚠️ Les fichiers `.ino` actuels utilisent GPIO 21 pour ECHO_2 sur tous les nœuds. Il faut les corriger pour les nœuds 1 et 2 qui ont un OLED sur GPIO 21.

**Modification dans `node1.ino` et `node2.ino` :**
```cpp
// Remplacer
const int ECHO_2 = 21;
// Par
const int ECHO_2 = 23;  // GPIO 21 réservé pour OLED SDA
```

**Ajout du contrôle LED dans tous les nœuds :**
```cpp
// Broches LED (ajouter en haut du fichier)
const int LED_1 = 2;
const int LED_2 = 4;

// Dans setup() ajouter :
pinMode(LED_1, OUTPUT);
pinMode(LED_2, OUTPUT);

// Dans loop() après les mesures, ajouter :
digitalWrite(LED_1, occ1 ? HIGH : LOW);  // LED allumée = place occupée
digitalWrite(LED_2, occ2 ? HIGH : LOW);
```

**Ajout de l'OLED dans node1.ino et node2.ino :**
```cpp
// Bibliothèque à installer : "Adafruit SSD1306" + "Adafruit GFX"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Dans setup() :
display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
display.clearDisplay();

// Fonction d'affichage (appeler après chaque envoi) :
void updateDisplay(bool occ1, bool occ2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("=== PARKING ===");
    display.printf("Place %d: %s\n", SPOT_ID_1, occ1 ? "OCCUPEE" : "LIBRE  ");
    display.printf("Place %d: %s\n", SPOT_ID_2, occ2 ? "OCCUPEE" : "LIBRE  ");
    display.display();
}
```

---

## Ordre de montage recommandé

1. **Commencer par un seul nœud** (nœud 1) — tester avant de multiplier
2. Brancher l'ESP32 en USB-C sur le PC
3. Câbler les **alimentations** (VIN et GND) en premier
4. Câbler un **seul capteur** HC-SR04A, flasher le code, tester avec le Serial Monitor
5. Ajouter le **second capteur**
6. Ajouter les **2 LEDs** avec résistances
7. Ajouter l'**OLED** (si nœud 1 ou 2)
8. Tester l'envoi des données au serveur Symfony (s'assurer que Symfony tourne)
9. Répéter pour les nœuds 2 et 3

---

## Tests rapides

### Tester un capteur HC-SR04A seul
Dans le Serial Monitor (115200 bauds), vous devez voir les distances s'afficher. Passez votre main devant le capteur — la valeur doit diminuer.

### Tester une LED
Temporairement dans `setup()` :
```cpp
digitalWrite(LED_1, HIGH); // Doit s'allumer
delay(1000);
digitalWrite(LED_1, LOW);  // Doit s'éteindre
```

### Tester l'envoi au serveur (sans ESP32)
```bash
curl -X POST http://127.0.0.1:8000/api/sensor/data \
  -H "Content-Type: application/json" \
  -H "X-API-Key: une_cle_secrete_longue_pour_esp32_changez_moi" \
  -d "{\"spotId\": 1, \"distanceCm\": 8.5, \"isOccupied\": true, \"esp32Id\": \"node-1\"}"
```

---

## Ce qu'il vous reste à acheter

| Composant | Pourquoi | Prix estimé |
|-----------|---------|-------------|
| **3 câbles USB-C** (ou chargeurs) | Alimenter les ESP32 pour la présentation | ~5€ chacun |
| **Petites breadboards** (optionnel) | Plus propre pour maintenir LED + résistances | ~3€ chacune |
| **Bibliothèques Arduino** (gratuit) | Adafruit SSD1306 + Adafruit GFX pour l'OLED | 0€ |
