import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function GET() {
  const spots = await prisma.parkingSpot.findMany({
    orderBy: { id: 'asc' },
    include: {
      reservations: {
        where: { status: { in: ['ACTIVE', 'EXPIRED'] } },
        orderBy: { endTime: 'desc' },
        take: 1,
      },
    },
  });

  const payload = spots.map((s) => ({
    id: s.id,
    name: s.name,
    is_occupied: s.isOccupied,
    last_updated_at: s.lastUpdatedAt.toISOString(),
    is_active: s.isActive,
    activeReservation: s.reservations[0] ?? null,
  }));

  return Response.json(payload);
}
