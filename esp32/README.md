# Code ESP32 — Parking Intelligent

## Matériel nécessaire
- 3x Carte NodeMCU ESP32
- 6x Capteur ultrason HC-SR04A
- Jumpers, résistances, breadboard

## Bibliothèques Arduino à installer

Outils → Gérer les bibliothèques → chercher et installer :
- **ArduinoJson** par Benoit Blanchon (version 6.x)
- **Adafruit SSD1306** par Adafruit
- **Adafruit GFX Library** par Adafruit

Ces bibliothèques sont nécessaires pour les nœuds 1 et 2 (OLED).
Le nœud 3 n'a pas besoin de Adafruit SSD1306 ni Adafruit GFX.

## Avant de flasher

1. Ouvrir le fichier `.ino` du nœud voulu dans Arduino IDE
2. Installer les bibliothèques nécessaires (Outils → Gérer les bibliothèques) :
   - **ArduinoJson** par Benoit Blanchon (v6.x)
   - **Adafruit SSD1306** par Adafruit (nœuds 1 & 2 uniquement)
   - **Adafruit GFX Library** par Adafruit (nœuds 1 & 2 uniquement)
   - **HTTPClient** (inclus avec le support ESP32)
3. Ajouter le support ESP32 dans Arduino IDE :
   - Fichier → Préférences → URL de gestionnaire de cartes supplémentaires :
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Outils → Type de carte → Gestionnaire de cartes → chercher "esp32" → Installer
4. Sélectionner la carte : Outils → Type de carte → ESP32 → **NodeMCU-32S** ou **ESP32 Dev Module**

## Configuration réseau — Hotspot Windows (recommandé)

Le PC crée lui-même un réseau WiFi. Les ESP32 s'y connectent directement.
Aucun routeur, aucune connexion internet, aucun réseau école requis.

### Activer le Hotspot sur le PC
1. Paramètres → Réseau et Internet → Point d'accès sans fil
2. Activer le point d'accès
3. Configurer :
   - Nom du réseau : `ParkingIntelligent`
   - Mot de passe : `parking2026`
   - Partager depuis : WiFi ou Ethernet (au choix)

**L'IP du PC est toujours `192.168.137.1` sur un hotspot Windows — aucune configuration supplémentaire.**

### Lancer Symfony
```bash
symfony server:start --no-tls
```
Le serveur écoute sur `http://192.168.137.1:8000` pour les ESP32
et sur `http://127.0.0.1:8000` pour le navigateur du PC.

### Les .ino sont déjà configurés
`SERVER_IP = "192.168.137.1"` est déjà en dur dans tous les fichiers.
Seul le flashage est nécessaire, pas de modification.

## Option B — Partage de connexion 4G (téléphone)

Si le hotspot PC ne fonctionne pas :
1. Activer le partage de connexion sur votre téléphone
2. Connecter le PC au réseau du téléphone
3. Faire `ipconfig` sur le PC → noter l'IP WiFi (ex: 192.168.43.x)
4. Modifier `SERVER_IP` dans les .ino avec cette IP
5. Flasher à nouveau les ESP32

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
