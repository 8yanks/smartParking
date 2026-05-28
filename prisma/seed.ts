import 'dotenv/config';
import { PrismaClient } from '../src/generated/prisma/client';
import { PrismaBetterSqlite3 } from '@prisma/adapter-better-sqlite3';

const adapter = new PrismaBetterSqlite3({
  url: process.env.DATABASE_URL ?? 'file:./prisma/dev.db',
});
const prisma = new PrismaClient({ adapter });

async function main() {
  // ── Places ──────────────────────────────────────────────────────────────
  // 5 places (matériel mis à jour : 5 capteurs au lieu de 6).
  // La place 5 = place handicapée (extrémité droite, là où il manque l'ancienne place 6).
  const spots = [
    { id: 1, name: 'Place A1', isHandicap: false },
    { id: 2, name: 'Place A2', isHandicap: false },
    { id: 3, name: 'Place A3', isHandicap: false },
    { id: 4, name: 'Place A4', isHandicap: false },
    { id: 5, name: 'Place A5', isHandicap: true },
  ];

  // Cleanup d'une éventuelle place 6 (et de ses enregistrements rattachés)
  // — utile pour les bases déjà seedées avec 6 places.
  await prisma.sensorData.deleteMany({ where: { spotId: 6 } });
  await prisma.reservation.deleteMany({ where: { spotId: 6 } });
  await prisma.parkingSpot.deleteMany({ where: { id: 6 } });

  for (const s of spots) {
    await prisma.parkingSpot.upsert({
      where: { id: s.id },
      update: { name: s.name, isHandicap: s.isHandicap },
      create: { id: s.id, name: s.name, isHandicap: s.isHandicap },
    });
  }

  // ── Barrière ────────────────────────────────────────────────────────────
  // Singleton (id = 1). Fermée par défaut.
  await prisma.barrier.upsert({
    where: { id: 1 },
    update: {},
    create: { id: 1, isOpen: false },
  });

  console.log('✓ Seed OK — 5 places (A5 = handicap) + barrière initialisée (fermée)');
}

main()
  .catch((e) => {
    console.error(e);
    process.exit(1);
  })
  .finally(async () => {
    await prisma.$disconnect();
  });
