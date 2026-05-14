# Smart Parking — Équipe 154

Site web local pour gérer un parking intelligent de **6 places** équipées de capteurs ultrasons HC-SR04A pilotés par 3 ESP32 NodeMCU.

> ⚠️ **Pivot stack en cours.** Le repo a été migré de Symfony 7 vers **Next.js 16 + Prisma 7 + SQLite**. L'ancien backend Symfony est préservé sur la branche [`archive/symfony`](../../tree/archive/symfony).

## Architecture

```
┌──────── ESP32 #1 "panneau" ────────┐
│  2 capteurs (places 1-2)           │
│  2 LEDs locales                    │
│  2 OLED SSD1306 I2C (panneau info) │
│  ▲ POST /api/spot/{1,2}            │
│  ▼ GET  /api/spots                 │
└──────────────┬─────────────────────┘
               │  HTTP + X-API-Key
[ESP32 #2 capteurs places 3-4] ──────┤
[ESP32 #3 capteurs places 5-6] ──────┤
                                     ▼
                          [Next.js 16 + Prisma + SQLite]
                          (PC en hotspot WiFi)
```

Voir [`docs/superpowers/specs/`](docs/superpowers/specs/) pour le design détaillé et [`esp32/README.md`](esp32/README.md) pour le câblage GPIO.

## Stack

| Brique | Choix |
|--------|-------|
| Backend + frontend | Next.js 16 (App Router, TypeScript) |
| ORM | Prisma 7 + adapter `better-sqlite3` |
| BDD | SQLite (fichier local) |
| UI | Tailwind CSS 4 |
| Code embarqué | Arduino C++ pour ESP32 |

## Setup local

### 1. Prérequis

- Node.js 20+ ([nodejs.org](https://nodejs.org))
- Arduino IDE 2.x avec board ESP32 + libs `ArduinoJson`, `Adafruit GFX`, `Adafruit SSD1306` (voir [`esp32/README.md`](esp32/README.md))
- Un PC avec **WiFi** pour faire hotspot vers les ESP32

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

> ⚠️ Cette clé doit être recopiée à l'identique dans les sketches `esp32/*.ino` (variable `API_KEY`).

### 4. Initialiser la base SQLite

```bash
npx prisma migrate deploy   # applique les migrations existantes
npx prisma generate          # génère le client TypeScript
npm run db:seed              # crée les 6 places A1-A6
```

### 5. Démarrer le serveur

```bash
npm run dev
```

Ouvre <http://localhost:3000>. L'API est disponible sur :
- `GET /api/spots` (publique, pour le dashboard)
- `POST /api/spot/{id}` (auth `X-API-Key`, pour les ESP32)

### 6. Tester l'API

```bash
# Liste les 6 places
curl http://localhost:3000/api/spots

# Mettre une place occupée (remplace TA_CLE par ESP32_API_KEY)
curl -X POST http://localhost:3000/api/spot/3 \
  -H "Content-Type: application/json" \
  -H "X-API-Key: TA_CLE" \
  -d '{"occupied":true,"distance_cm":12.5,"esp32_id":"node-test"}'
```

## Déploiement matériel

Voir [`esp32/README.md`](esp32/README.md) pour :
- Câblage GPIO de chaque nœud (capteurs + LEDs + OLEDs)
- Configuration des sketches (WiFi, URL serveur, clé API)
- Étapes de flash via Arduino IDE

## Scripts npm

| Commande | Effet |
|----------|-------|
| `npm run dev` | Démarre Next.js en mode dev (port 3000) |
| `npm run build` | Build production |
| `npm run start` | Démarre le build production |
| `npm run db:seed` | Réinitialise les 6 places en BDD |
| `npm run lint` | Vérifie le code avec ESLint |

## Structure du projet

```
smartParking/
├── docs/superpowers/         ← design + plan d'implémentation
├── esp32/                    ← sketches Arduino + README câblage
│   ├── node-1.ino            ← nœud panneau (2 OLEDs I2C, double-bus)
│   ├── node-sensor.ino       ← nœuds 2 et 3 (capteurs simples)
│   └── README.md
├── prisma/
│   ├── schema.prisma         ← modèle BDD
│   ├── seed.ts               ← seed 6 places
│   └── migrations/
└── src/
    ├── app/
    │   ├── api/
    │   │   ├── spot/[id]/route.ts   ← POST ESP32
    │   │   └── spots/route.ts       ← GET dashboard
    │   ├── layout.tsx
    │   └── page.tsx
    ├── lib/prisma.ts                ← singleton Prisma
    └── generated/prisma/            ← client généré (gitignored)
```
