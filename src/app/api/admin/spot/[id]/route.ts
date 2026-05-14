import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function POST(
  request: NextRequest,
  { params }: { params: Promise<{ id: string }> }
) {
  const { id } = await params;
  const spotId = Number(id);
  if (!Number.isInteger(spotId)) {
    return Response.json({ error: 'Invalid spot id' }, { status: 400 });
  }

  let body: { occupied: boolean };
  try {
    body = await request.json();
  } catch {
    return Response.json({ error: 'Invalid JSON' }, { status: 400 });
  }

  const spot = await prisma.parkingSpot.update({
    where: { id: spotId },
    data: { isOccupied: body.occupied },
  });

  // Détection dépassement : 5€ base + 0,50€ par minute de dépassement
  if (body.occupied) {
    const now = new Date();
    const expired = await prisma.reservation.findFirst({
      where: {
        spotId,
        status: { in: ['ACTIVE', 'EXPIRED'] },
        endTime: { lt: now },
      },
      orderBy: { endTime: 'desc' },
    });
    if (expired) {
      const minutesOver = (now.getTime() - expired.endTime.getTime()) / 60_000;
      const fineAmount = Math.round((5 + minutesOver * 0.5) * 100) / 100;
      await prisma.reservation.update({
        where: { id: expired.id },
        data: { status: 'EXPIRED', fineAmount },
      });
    }
  }

  return Response.json({ ok: true, spot: { id: spot.id, isOccupied: spot.isOccupied } });
}
