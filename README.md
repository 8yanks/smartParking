# Smart Parking — Équipe 112

Site web local pour gérer un parking intelligent de **5 places** (dont 1 handicap) équipées de capteurs ultrasons HC-SR04A. La barrière à l'entrée est purement physique (non motorisée). Stack : Next.js 16 + Prisma 7 + SQLite côté serveur, ESP32 NodeMCU côté terrain.

> 🗄️ L'ancien backend Symfony est archivé sur la branche [`archive/symfony`](../../tree/archive/symfony).

## Architecture

```
┌──────── ESP32 #1 "panneau d'entrée" ──────────┐
│  2 capteurs (places 1-2)                      │
│  2 LEDs locales                               │
│  2 OLED SSD1306 I2C (panneau info)            │
│  ▲ POST /api/spot/{1,2}                       │
│  ▼ GET  /api/spots                            │
└──────────────┬────────────────────────────────┘
               │  HTTP + X-API-Key
[ESP32 #2 capteurs places 3-4] ─────────────────┤
[ESP32 #3 capteur place 5 (handicap ♿)] ───────┤
                                                ▼
                            ┌─────────────────────────────────┐
                            │ Next.js 16 + Prisma + SQLite    │
                            │ Dashboard ATM + admin           │
                            │ Réservations + amendes          │
                            └─────────────────────────────────┘
                            (PC en hotspot WiFi 2.4 GHz)
```

Détails : design dans [`docs/superpowers/specs/`](docs/superpowers/specs/), câblage GPIO pas à pas dans [`esp32/WIRING.md`](esp32/WIRING.md), transition machine dans [`docs/HANDOFF.md`](docs/HANDOFF.md).

## Stack

| Brique | Choix |
|--------|-------|
| Backend + frontend | Next.js 16 (App Router, TypeScript) |
| ORM | Prisma 7 + adapter `better-sqlite3` |
| BDD | SQLite (fichier local) |
| UI | Tailwind CSS 4 |
| Code embarqué | Arduino C++ pour ESP32 (libs WiFi, HTTPClient, ArduinoJson, Adafruit GFX/SSD1306) |

## Setup local

### 1. Prérequis

- **Node.js 20+** ([nodejs.org](https://nodejs.org))
- **Arduino IDE 2.x** avec board ESP32 + libs `ArduinoJson`, `Adafruit GFX`, `Adafruit SSD1306` (voir [`esp32/README.md`](esp32/README.md))
- Un PC avec **WiFi** pour faire hotspot vers les ESP32 (2.4 GHz obligatoire)

### 2. Installer le serveur

```bash
git clone https://github.com/8yanks/smartParking.git
cd smartParking
npm install
```

### 3. Configurer les variables d'environnement

```bash
cp .env.example .env
```

Génère une clé API et remplace `ESP32_API_KEY` dans `.env` :

```bash
node -e "console.log(require('node:crypto').randomBytes(32).toString('hex'))"
```

> ⚠️ Cette clé doit être recopiée à l'identique dans les sketches `esp32/**/*.ino` (variable `API_KEY`). Si tu changes la clé, tu dois reflasher les 3 ESP32.

### 4. Initialiser la base SQLite

```bash
npx prisma migrate deploy   # applique toutes les migrations
npx prisma generate         # génère le client TypeScript
npm run db:seed             # crée les 5 places A1-A5 (A5 = handicap)
```

### 5. Démarrer le serveur

```bash
npm run dev                  # tests locaux uniquement (localhost)
npm run dev -- -H 0.0.0.0    # accessible aussi depuis le hotspot (ESP32)
```

Ouvre <http://localhost:3000>.

### 6. Tester l'API

```bash
# Liste les 5 places (avec is_handicap)
curl http://localhost:3000/api/spots

# Mettre une place occupée (remplace TA_CLE par ESP32_API_KEY)
curl -X POST http://localhost:3000/api/spot/3 \
  -H "Content-Type: application/json" \
  -H "X-API-Key: TA_CLE" \
  -d '{"occupied":true,"distance_cm":12.5,"esp32_id":"node-test"}'
```

## Endpoints HTTP

| Méthode | Route | Auth | Usage |
|---------|-------|------|-------|
| `GET` | `/api/spots` | publique | Liste des 5 places (avec `is_handicap`, réservation active) — utilisé par dashboard + nœud #1 |
| `POST` | `/api/spot/{1..5}` | `X-API-Key` | ESP32 → état d'une place (occupé/libre + distance + esp32_id) |
| `GET`/`POST` | `/api/reservations` | publique | Créer/lire une réservation (plaque + place + durée + montant) |
| `POST` | `/api/admin/*` | publique (local) | Outils admin (override capteur, reset DB) |

## Déploiement matériel

Suivre le guide pas à pas [`esp32/WIRING.md`](esp32/WIRING.md) — du "Hello World" jusqu'au câblage complet des 3 nœuds. Topologie résumée :

| Nœud | Matériel | Sketch |
|------|----------|--------|
| #1 panneau | 2 capteurs + 2 LEDs + 2 OLED I2C | `esp32/node-1/node-1.ino` |
| #2 | 2 capteurs (places 3, 4) + 2 LEDs | `esp32/node-sensor/node-sensor.ino` (bloc node-2) |
| #3 | **1 capteur** (place 5 handicap) + 1 LED | `esp32/node-sensor/node-sensor.ino` (bloc node-3, `SPOT_ID_B = -1`) |

## Scripts npm

| Commande | Effet |
|----------|-------|
| `npm run dev` | Démarre Next.js en mode dev (port 3000, localhost uniquement) |
| `npm run dev -- -H 0.0.0.0` | Idem, accessible depuis le hotspot (ESP32) |
| `npm run build` | Build production |
| `npm run start` | Démarre le build production |
| `npm run db:seed` | Réinitialise les 5 places |
| `npm run lint` | Vérifie le code avec ESLint |

## Structure du projet

```
smartParking/
├── docs/
│   ├── HANDOFF.md            ← guide migration sur une autre machine
│   └── superpowers/          ← specs design + plan d'implémentation
├── esp32/
│   ├── node-1/node-1.ino     ← nœud panneau (2 OLEDs I2C)
│   ├── node-sensor/node-sensor.ino  ← sketch partagé nœuds #2 et #3
│   ├── WIRING.md             ← câblage pas à pas
│   └── README.md             ← récap pinout + API
├── prisma/
│   ├── schema.prisma         ← ParkingSpot (isHandicap), Reservation, SensorData
│   ├── seed.ts               ← 5 places (A5 = handicap)
│   └── migrations/
└── src/
    ├── app/
    │   ├── api/
    │   │   ├── spot/[id]/route.ts      ← POST ESP32 (auth)
    │   │   ├── spots/route.ts          ← GET dashboard
    │   │   ├── reservations/route.ts   ← POST réservations
    │   │   └── admin/                  ← endpoints tests
    │   ├── admin/page.tsx              ← panneau admin (toggle place, reset DB)
    │   ├── layout.tsx
    │   └── page.tsx                    ← interface ATM (plaque → grille → réserver)
    ├── lib/prisma.ts                   ← singleton Prisma
    └── generated/prisma/               ← client généré (gitignored)
```

## Pièges connus

- **Next.js 16** : dans les route handlers dynamiques, `params` est un `Promise<...>` — toujours `await` (`const { id } = await params`).
- **Prisma 7** : `new PrismaClient()` exige un adapter explicite (déjà géré dans `src/lib/prisma.ts`).
- **Client Prisma** dans `src/generated/prisma` (gitignored) — relancer `npx prisma generate` après chaque clone.
- **ESP32 + WiFi 5 GHz** : ne fonctionne pas, hotspot Windows en **2.4 GHz obligatoire**.
- **`npm run dev` simple** = écoute sur `localhost`, donc les ESP32 ne voient pas. Utiliser `-H 0.0.0.0` pour exposer sur toutes les interfaces.
