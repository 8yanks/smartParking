import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function POST() {
  await prisma.reservation.deleteMany({});
  await prisma.parkingSpot.updateMany({ data: { isOccupied: false } });
  return Response.json({ ok: true });
}
