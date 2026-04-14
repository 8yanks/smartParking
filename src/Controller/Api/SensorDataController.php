<?php

namespace App\Controller\Api;

use App\Entity\SensorData;
use App\Repository\ParkingSpotRepository;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/api', name: 'api_')]
class SensorDataController extends AbstractController
{
    #[Route('/sensor/data', name: 'sensor_data_post', methods: ['POST'])]
    public function receiveData(
        Request $request, 
        ParkingSpotRepository $parkingSpotRepository, 
        EntityManagerInterface $em
    ): JsonResponse {
        $content = $request->getContent();
        $data = json_decode($content, true);

        if (!$data || !isset($data['spotId']) || !isset($data['distanceCm']) || !isset($data['isOccupied'])) {
            return $this->json(['error' => 'Invalid JSON payload'], Response::HTTP_BAD_REQUEST);
        }

        $spot = $parkingSpotRepository->find($data['spotId']);

        if (!$spot) {
            return $this->json(['error' => 'Parking spot not found'], Response::HTTP_NOT_FOUND);
        }

        // Création de l'historique SensorData
        $sensorData = new SensorData();
        $sensorData->setParkingSpot($spot);
        $sensorData->setDistanceCm($data['distanceCm']);
        $sensorData->setIsOccupied($data['isOccupied']);
        $sensorData->setEsp32Id($data['esp32Id'] ?? 'unknown');
        $sensorData->setRecordedAt(new \DateTimeImmutable());

        // Mise à jour de la place de parking
        $spot->setIsOccupied($data['isOccupied']);
        // If there were a setLastUpdatedAt, we would call it here. The ParkingSpot entity has it.
        $spot->setLastUpdatedAt(new \DateTimeImmutable());

        $em->persist($sensorData);
        $em->flush();

        return $this->json([
            'message' => 'Sensor data recorded successfully',
            'spotId' => $spot->getId(),
            'isOccupied' => $spot->isOccupied()
        ], Response::HTTP_CREATED);
    }

    #[Route('/parking/status', name: 'parking_status', methods: ['GET'])]
    public function getParkingStatus(ParkingSpotRepository $parkingSpotRepository): JsonResponse
    {
        $spots = $parkingSpotRepository->findAll();
        $data = [];

        foreach ($spots as $spot) {
            $data[] = [
                'id' => $spot->getId(),
                'name' => $spot->getName(),
                'isOccupied' => $spot->isOccupied(),
                'isActive' => $spot->isActive()
            ];
        }

        return $this->json($data);
    }
}
