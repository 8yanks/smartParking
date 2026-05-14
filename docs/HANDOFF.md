# Handoff — État du projet au 2026-05-14

Document de transition pour la prochaine session Claude (côté PC portable de l'utilisateur).

## Lis d'abord

1. [`README.md`](../README.md) — setup local + architecture
2. [`docs/superpowers/specs/2026-04-14-parking-intelligent-design.md`](superpowers/specs/2026-04-14-parking-intelligent-design.md) — design complet (mis à jour avec les vrais composants matériels)
3. [`esp32/README.md`](../esp32/README.md) — câblage GPIO de chaque nœud

## Décisions de stack déjà prises (ne pas re-questionner)

- **Pivot Symfony → Next.js validé par l'utilisateur.** Symfony archivé sur la branche `archive/symfony`.
- **Stack** : Next.js 16 (App Router, TypeScript) + Prisma 7 + adapter `better-sqlite3` + Tailwind 4.
- **Exécution 100 % locale** : le PC portable de l'utilisateur fait hotspot WiFi, les ESP32 s'y connectent.
- **HTTP, pas HTTPS** (local).

## État des phases

| Phase | État | Fait sur PC fixe |
|-------|------|------------------|
| 0. Prérequis (Node, Arduino IDE) | ✅ Node OK / Arduino à installer côté user | partiel |
| 1. Backend Next.js + API | ✅ **Tests curl OK (POST 401/200, GET 200)** | sur PC fixe (transféré via git) |
| 2. Hotspot Windows + firewall | ❌ À faire **sur le portable** (PC fixe sans WiFi) | — |
| 3. Test smoke 1 ESP32 | ❌ À faire | — |
| 4. Montage des 3 nœuds | ❌ À faire | — |
| 5. Dashboard temps réel | ❌ À faire | — |
| 6. Auth + réservations + Stripe (optionnel) | ❌ À faire | — |

## Matériel (rappel — tout est dans esp32/README.md)

- 3× ESP32 NodeMCU
- 6× capteur HC-SR04A (un par place)
- 6× LED rouge + R 220Ω (1 par place, allumée si occupée)
- **2× OLED SBC-OLED01-V2 (SSD1306) en SPI**, montés ensemble sur le **nœud #1** = panneau d'info à l'entrée
- 1× LED rouge spare
- Breadboards + jumpers + câbles micro-USB OK côté utilisateur

## Pièges techniques découverts (Next.js 16 / Prisma 7)

1. **Next.js 16** : dans les route handlers dynamiques, `params` est un `Promise<...>` — toujours `await`.
   ```ts
   export async function POST(req: NextRequest, { params }: { params: Promise<{ id: string }> }) {
     const { id } = await params;
   }
   ```

2. **Prisma 7** : impossible de faire `new PrismaClient()` sans options. Il FAUT un adapter explicite. Pour SQLite :
   ```ts
   import { PrismaClient } from '@/generated/prisma/client';
   import { PrismaBetterSqlite3 } from '@prisma/adapter-better-sqlite3';
   const adapter = new PrismaBetterSqlite3({ url: process.env.DATABASE_URL });
   const prisma = new PrismaClient({ adapter });
   ```

3. **Prisma 7 client généré dans `src/generated/prisma`** (gitignored). Il faut `npx prisma generate` après chaque clone.

4. **Le `.env` n'est pas commité** — l'utilisateur doit copier `.env.example` et générer une clé API. Les sketches Arduino doivent utiliser **la même clé**.

5. **Le seed nécessite `dotenv`** car il s'exécute hors du contexte Next.js. Déjà géré dans `prisma/seed.ts`.

## Ce que la prochaine session doit faire

### Étape A — Vérifier le setup local (5 min)

```bash
npm install
cp .env.example .env
node -e "console.log(require('node:crypto').randomBytes(32).toString('hex'))"  # → coller dans .env
npx prisma migrate deploy
npx prisma generate
npm run db:seed
npm run dev
# Tester GET http://localhost:3000/api/spots → doit retourner 6 places
```

### Étape B — Phase 2 : Hotspot + firewall

Sur le PC portable Windows, avec WiFi actif :
1. Activer Mobile hotspot via `Settings → Network → Mobile hotspot` (band 2.4 GHz **obligatoire**).
2. Récupérer l'IP du PC sur le hotspot :
   ```powershell
   Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.InterfaceAlias -like "*Local Area*" }
   ```
   (typiquement `192.168.137.1`)
3. Ouvrir le port 3000 dans le firewall (PowerShell admin) :
   ```powershell
   New-NetFirewallRule -DisplayName "Next.js dev (3000)" -Direction Inbound -LocalPort 3000 -Protocol TCP -Action Allow
   ```
4. Lancer Next.js en écoute sur toutes les interfaces :
   ```bash
   npm run dev -- -H 0.0.0.0
   ```
5. Depuis un autre device sur le hotspot, tester `http://192.168.137.1:3000/api/spots`.

### Étape C — Adapter les 2 sketches Arduino

Dans `esp32/node-1.ino` ET `esp32/node-sensor.ino`, remplacer :
```cpp
const char* WIFI_SSID     = "YKS_0221";          // SSID du hotspot user
const char* WIFI_PASSWORD = "smartParking";       // password user
const char* SERVER_URL    = "http://192.168.137.1:3000";  // IP du PC + port (PAS de https !)
const char* API_KEY       = "<la clé du .env>";
```

### Étape D et suite

Phases 3–6 selon le plan dans [`docs/superpowers/plans/2026-04-14-parking-intelligent.md`](superpowers/plans/2026-04-14-parking-intelligent.md) (mais ce plan est encore en français Symfony — à réécrire en Next.js si tu veux le garder à jour).

## Contexte utilisateur

- Étudiant, équipe 154 du projet "Parking Intelligent" (membres : Yanis Michaux, Théo Perret, Adam Bruneau, James Malespine).
- Niveau ESP32 : "quelques bases".
- Préférences : décisions cadrées (utilise `AskUserQuestion` pour les choix importants), réponses concises, pas de pavé.
- Session précédente : tout le bootstrap a été fait sur PC fixe, transféré via Git.
