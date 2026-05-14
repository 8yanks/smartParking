# Migration vers le PC portable — Guide pas à pas

Tu es **sur le PC portable** (celui qui fait hotspot WiFi + sert Next.js + héberge la base SQLite). Suis ce guide dans l'ordre. Compter **~45 min** pour avoir le premier ESP32 qui POST sur le serveur.

> Tout est déjà codé côté serveur et côté ESP32 — il n'y a que de la **config** à faire.

---

## Phase 0 — Prérequis logiciels (10 min)

À installer **une fois** sur le portable, dans cet ordre :

| Outil | Pourquoi | Où |
|-------|----------|-----|
| **Node.js 20+** | Pour faire tourner Next.js + Prisma | <https://nodejs.org> (installer LTS) |
| **Git** | Pour cloner le repo | <https://git-scm.com> (ou déjà installé) |
| **Arduino IDE 2.x** | Pour flasher les ESP32 | <https://www.arduino.cc/en/software> |
| **Drivers USB-Serial** | Pour que Windows voie l'ESP32 | **CP210x** : <https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers> · **CH340** : <https://sparks.gogo.co.nz/ch340.html> (selon la puce de ta board — branche un ESP32 et regarde le gestionnaire de périphériques) |

Vérifications rapides en PowerShell :
```powershell
node --version    # v20.x ou +
npm --version
git --version
```

### Dans l'Arduino IDE

1. **File → Preferences → Additional boards manager URLs** : ajouter
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. **Tools → Board → Boards Manager** → chercher `esp32` → installer **esp32 by Espressif Systems**.
3. **Tools → Manage Libraries** → installer :
   - `ArduinoJson` (Benoit Blanchon)
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`

---

## Phase 1 — Récupérer le repo (5 min)

```powershell
cd $env:USERPROFILE\Desktop
git clone https://github.com/8yanks/smartParking.git
cd smartParking
npm install
```

`npm install` prend ~2 min (Next.js + Prisma + better-sqlite3 compile une native binding).

---

## Phase 2 — Configurer le `.env` (3 min)

Le `.env` n'est **pas** dans le repo (gitignored à cause de la clé API). Tu as deux options :

### Option A — Reprendre la clé du PC fixe (recommandé)
Copie le `.env` du PC fixe vers le portable (clé USB, mail à toi-même, etc.). La clé reste la même → **pas besoin de reflasher les sketches** si tu les as déjà compilés.

### Option B — Générer une nouvelle clé
```powershell
Copy-Item .env.example .env
$key = node -e "console.log(require('node:crypto').randomBytes(32).toString('hex'))"
(Get-Content .env) -replace 'ESP32_API_KEY=.*', "ESP32_API_KEY=`"$key`"" | Set-Content .env
Write-Output "Clé générée : $key"
```

⚠️ Note bien la clé — il faut la coller dans les 3 sketches ESP32 (variable `API_KEY` en haut de chaque `.ino`).

---

## Phase 3 — Initialiser la base SQLite (2 min)

```powershell
npx prisma migrate deploy   # applique la migration init
npx prisma generate         # génère le client Prisma dans src/generated/prisma
npm run db:seed             # crée les 6 places A1-A6
```

Vérification :
```powershell
npm run dev
# Dans un autre terminal :
curl http://localhost:3000/api/spots
```
Tu dois voir un JSON avec 6 places. Stoppe avec `Ctrl+C`.

---

## Phase 4 — Hotspot WiFi + firewall (5 min)

1. **Activer le hotspot** : Paramètres Windows → Réseau et Internet → Mobile hotspot.
   - Bande : **2.4 GHz obligatoire** (les ESP32 ne voient pas le 5 GHz).
   - Note bien le **SSID** et le **mot de passe** affichés.

2. **Vérifier l'IP du portable** sur le hotspot (PowerShell) :
   ```powershell
   Get-NetIPAddress -AddressFamily IPv4 |
     Where-Object { $_.InterfaceAlias -match 'Local Area Connection' -or $_.InterfaceAlias -match 'Connexion' } |
     Select-Object IPAddress, InterfaceAlias
   ```
   Typiquement **`192.168.137.1`**. Si différent, note l'IP — il faudra l'utiliser dans les sketches.

3. **Ouvrir le port 3000 dans le firewall** (PowerShell **admin**, une fois pour toutes) :
   ```powershell
   New-NetFirewallRule -DisplayName "Next.js parking" -Direction Inbound -LocalPort 3000 -Protocol TCP -Action Allow -Profile Any
   ```

4. **Lancer le serveur en écoute sur toutes les interfaces** :
   ```powershell
   npm run dev -- -H 0.0.0.0
   ```
   Garde ce terminal ouvert pendant toute la phase ESP32.

---

## Phase 5 — Configurer et flasher les ESP32 (15 min)

Tous les détails de câblage sont dans [`esp32/WIRING.md`](../esp32/WIRING.md) — suis les **8 étapes** (Hello World → panneau complet).

Avant le flash, édite dans chaque sketch ces 4 constantes :

```cpp
const char* WIFI_SSID     = "<SSID de ton hotspot>";        // ex: YKS_0221
const char* WIFI_PASSWORD = "<mot de passe du hotspot>";
const char* SERVER_URL    = "http://192.168.137.1:3000";    // adapter si IP différente
const char* API_KEY       = "<la valeur de ESP32_API_KEY du .env>";
```

Pour `node-sensor.ino`, choisir aussi le bon bloc selon le nœud :
- ESP32 #2 → `NODE_ID = "node-2"`, `SPOT_ID_A = 3`, `SPOT_ID_B = 4`
- ESP32 #3 → `NODE_ID = "node-3"`, `SPOT_ID_A = 5`, `SPOT_ID_B = 6`

**Test smoke** : avec **un seul** ESP32 branché USB + un seul capteur sur GPIO 25/26 + une LED sur GPIO 32 (pinout dans WIRING.md), flashe et ouvre le moniteur série 115200 baud. Tu dois voir :
```
[node-2] Boot capteur
[WiFi] OK 192.168.137.X
[POST] place 3 LIBRE (47.3 cm) HTTP 200
```

`HTTP 200` = ✅ tout fonctionne bout-à-bout.

---

## Phase 6 — Suite (optionnelle pour l'instant)

Une fois les 3 nœuds en place et le dashboard à venir : voir [`docs/superpowers/plans/2026-04-14-parking-intelligent.md`](superpowers/plans/2026-04-14-parking-intelligent.md) (ce plan est encore tourné Symfony, à réécrire en Next.js si tu veux le poursuivre).

---

## Récap fichiers utiles

| Fichier | Quand le lire |
|---------|---------------|
| `README.md` (racine) | Vue d'ensemble du projet |
| `docs/HANDOFF.md` (ce fichier) | Migration sur portable |
| `esp32/WIRING.md` | Câblage pas à pas — 8 étapes guidées |
| `esp32/README.md` | Récap pinout + API attendue |
| `prisma/schema.prisma` | Modèle BDD (ParkingSpot, SensorData) |
| `src/app/api/spot/[id]/route.ts` | Endpoint POST que les ESP32 appellent |
| `src/app/api/spots/route.ts` | Endpoint GET pour le dashboard + nœud #1 |

---

## Pièges connus

- **`HTTP -1` côté ESP32** = pas de connexion au serveur. Vérifier : SSID/password, hotspot bien 2.4 GHz, firewall ouvert, serveur lancé avec `-H 0.0.0.0`.
- **`HTTP 401`** = clé API mauvaise. Vérifier que `API_KEY` du sketch == `ESP32_API_KEY` du `.env`.
- **OLED `init KO`** = mauvais bus I2C ou alim (3.3V obligatoire, pas 5V).
- **Capteur HC-SR04A renvoie -1** = pas en 5V ou TRIG/ECHO inversés.
- **Upload Arduino échoue "Failed to connect"** = maintiens le bouton **BOOT** de l'ESP32 pendant l'upload.
- **Next.js 16 + Prisma 7** : si tu touches au code, `params` est un `Promise<...>` à `await`, et `PrismaClient` exige un adapter (déjà gérés dans le code actuel).

---

## Contexte rapide

- Projet scolaire "Parking Intelligent" — équipe 154 (Yanis Michaux, Théo Perret, Adam Bruneau, James Malespine).
- Stack figée : Next.js 16 + Prisma 7 + SQLite + Tailwind 4. Symfony archivé sur la branche `archive/symfony`.
- Exécution 100% locale via hotspot Windows — pas de déploiement cloud prévu.
