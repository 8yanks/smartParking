import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

/**
 * État de la barrière d'entrée (servomoteur sur le node-1).
 *
 * - GET  : public. Lu par le dashboard ET par le node-1 (poll régulier).
 * - POST : public, contrôle local depuis le dashboard.
 *          Pas d'auth car le serveur n'est exposé qu'au hotspot WiFi local
 *          (cf. /api/admin/spot/[id] qui suit le même modèle).
 *
 * Body POST : `{ "isOpen": true | false }`
 */

async function getOrInitBarrier() {
  return prisma.barrier.upsert({
    where: { id: 1 },
    update: {},
    create: { id: 1, isOpen: false },
  });
}

export async function GET() {
  const barrier = await getOrInitBarrier();
  return Response.json({
    is_open: barrier.isOpen,
    updated_at: barrier.updatedAt.toISOString(),
  });
}

export async function POST(request: NextRequest) {
  let body: { isOpen?: unknown };
  try {
    body = await request.json();
  } catch {
    return Response.json({ error: 'Invalid JSON' }, { status: 400 });
  }

  if (typeof body.isOpen !== 'boolean') {
    return Response.json({ error: 'Field `isOpen` (boolean) required' }, { status: 400 });
  }

  const barrier = await prisma.barrier.upsert({
    where: { id: 1 },
    update: { isOpen: body.isOpen },
    create: { id: 1, isOpen: body.isOpen },
  });

  return Response.json({
    is_open: barrier.isOpen,
    updated_at: barrier.updatedAt.toISOString(),
  });
}
