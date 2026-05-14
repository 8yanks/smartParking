import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function GET() {
  const reservations = await prisma.reservation.findMany({
    orderBy: { createdAt: 'desc' },
    take: 50,
    include: { spot: { select: { name: true } } },
  });
  return Response.json(reservations);
}

export async function POST(request: NextRequest) {
  let body: { spotId: number; licensePlate?: string; durationSeconds: number; amountPaid?: number };
  try {
    body = await request.json();
  } catch {
    return Response.json({ error: 'Invalid JSON' }, { status: 400 });
  }

  const { spotId, licensePlate, durationSeconds, amountPaid } = body;
  if (!spotId || !durationSeconds || durationSeconds < 1) {
    return Response.json({ error: 'spotId et durationSeconds requis' }, { status: 400 });
  }

  // Auto-cancel any existing ACTIVE reservation on the same spot
  await prisma.reservation.updateMany({
    where: { spotId, status: 'ACTIVE' },
    data: { status: 'CANCELLED' },
  });

  const now = new Date();
  const endTime = new Date(now.getTime() + durationSeconds * 1000);
  const durationMinutes = Math.max(1, Math.ceil(durationSeconds / 60));

  const reservation = await prisma.reservation.create({
    data: {
      spotId,
      licensePlate: (licensePlate || 'TEST-001').toUpperCase(),
      durationMinutes,
      amountPaid: amountPaid ?? 0.01,
      endTime,
      status: 'ACTIVE',
    },
  });

  return Response.json(reservation, { status: 201 });
}
