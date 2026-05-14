import { NextRequest } from 'next/server';
import { prisma } from '@/lib/prisma';

type Body = {
  occupied: boolean;
  distance_cm: number;
  esp32_id: string;
};

export async function POST(
  request: NextRequest,
  { params }: { params: Promise<{ id: string }> }
) {
  // Auth — clé API partagée avec les ESP32
  const apiKey = request.headers.get('x-api-key');
  if (!apiKey || apiKey !== process.env.ESP32_API_KEY) {
    return Response.json({ error: 'Unauthorized' }, { status: 401 });
  }

  const { id } = await params;
  const spotId = Number(id);
  if (!Number.isInteger(spotId) || spotId < 1 || spotId > 6) {
    return Response.json({ error: 'Invalid spot id' }, { status: 400 });
  }

  let body: Body;
  try {
    body = (await request.json()) as Body;
  } catch {
    return Response.json({ error: 'Invalid JSON' }, { status: 400 });
  }

  if (typeof body.occupied !== 'boolean' || typeof body.distance_cm !== 'number' || typeof body.esp32_id !== 'string') {
    return Response.json({ error: 'Invalid body shape' }, { status: 400 });
  }

  // Mise à jour de l'état + log de la mesure (transaction)
  const [spot] = await prisma.$transaction([
    prisma.parkingSpot.update({
      where: { id: spotId },
      data: { isOccupied: body.occupied },
    }),
    prisma.sensorData.create({
      data: {
        spotId,
        isOccupied: body.occupied,
        distanceCm: body.distance_cm,
        esp32Id: body.esp32_id,
      },
    }),
  ]);

  return Response.json({ ok: true, spot: { id: spot.id, isOccupied: spot.isOccupied } });
}
