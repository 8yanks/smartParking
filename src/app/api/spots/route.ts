import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function GET() {
  const spots = await prisma.parkingSpot.findMany({
    orderBy: { id: 'asc' },
    select: {
      id: true,
      name: true,
      isOccupied: true,
      lastUpdatedAt: true,
      isActive: true,
    },
  });

  // Format compatible avec le code Arduino (snake_case attendu)
  const payload = spots.map((s) => ({
    id: s.id,
    name: s.name,
    is_occupied: s.isOccupied,
    last_updated_at: s.lastUpdatedAt.toISOString(),
    is_active: s.isActive,
  }));

  return Response.json(payload);
}
