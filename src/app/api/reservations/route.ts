import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function GET(request: NextRequest) {
  const plate = request.nextUrl.searchParams.get('plate');
  if (!plate) return Response.json({ error: 'plate required' }, { status: 400 });

  const reservations = await prisma.reservation.findMany({
    where: { licensePlate: plate.toUpperCase() },
    orderBy: { createdAt: 'desc' },
    take: 10,
    include: { spot: { select: { name: true } } },
  });

  return Response.json(reservations);
}

export async function POST(request: NextRequest) {
  let body: { spotId: number; licensePlate: string; durationMinutes: number; amountPaid: number };
  try {
    body = await request.json();
  } catch {
    return Response.json({ error: 'Invalid JSON' }, { status: 400 });
  }

  const { spotId, licensePlate, durationMinutes, amountPaid } = body;
  if (!spotId || !licensePlate || !durationMinutes || !amountPaid) {
    return Response.json({ error: 'Champs manquants' }, { status: 400 });
  }

  const spot = await prisma.parkingSpot.findUnique({ where: { id: spotId } });
  if (!spot) return Response.json({ error: 'Place introuvable' }, { status: 404 });

  const now = new Date();

  // Spot already reserved by someone else?
  const spotConflict = await prisma.reservation.findFirst({
    where: { spotId, status: 'ACTIVE', endTime: { gt: now } },
  });
  if (spotConflict) {
    return Response.json({ error: 'Cette place est déjà réservée' }, { status: 409 });
  }

  // This plate already has an active reservation?
  const plateConflict = await prisma.reservation.findFirst({
    where: { licensePlate: licensePlate.toUpperCase(), status: 'ACTIVE', endTime: { gt: now } },
  });
  if (plateConflict) {
    return Response.json({ error: 'Vous avez déjà une réservation active' }, { status: 409 });
  }

  const endTime = new Date(now.getTime() + durationMinutes * 60_000);

  const reservation = await prisma.reservation.create({
    data: {
      spotId,
      licensePlate: licensePlate.toUpperCase(),
      durationMinutes,
      amountPaid,
      endTime,
      status: 'ACTIVE',
    },
  });

  return Response.json(reservation, { status: 201 });
}
