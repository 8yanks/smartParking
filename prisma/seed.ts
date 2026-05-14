import 'dotenv/config';
import { PrismaClient } from '../src/generated/prisma/client';
import { PrismaBetterSqlite3 } from '@prisma/adapter-better-sqlite3';

const adapter = new PrismaBetterSqlite3({
  url: process.env.DATABASE_URL ?? 'file:./prisma/dev.db',
});
const prisma = new PrismaClient({ adapter });

async function main() {
  const spots = [
    { id: 1, name: 'Place A1' },
    { id: 2, name: 'Place A2' },
    { id: 3, name: 'Place A3' },
    { id: 4, name: 'Place A4' },
    { id: 5, name: 'Place A5' },
    { id: 6, name: 'Place A6' },
  ];

  for (const s of spots) {
    await prisma.parkingSpot.upsert({
      where: { id: s.id },
      update: { name: s.name },
      create: { id: s.id, name: s.name },
    });
  }

  console.log('✓ Seed OK — 6 places créées/maj');
}

main()
  .catch((e) => {
    console.error(e);
    process.exit(1);
  })
  .finally(async () => {
    await prisma.$disconnect();
  });
