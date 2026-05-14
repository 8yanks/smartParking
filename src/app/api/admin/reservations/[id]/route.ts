import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

export async function DELETE(
  _request: NextRequest,
  { params }: { params: Promise<{ id: string }> }
) {
  const { id } = await params;
  const reservationId = Number(id);
  if (!Number.isInteger(reservationId)) {
    return Response.json({ error: 'Invalid id' }, { status: 400 });
  }
  await prisma.reservation.delete({ where: { id: reservationId } }).catch(() => null);
  return Response.json({ ok: true });
}
