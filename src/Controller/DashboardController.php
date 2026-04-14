<?php

namespace App\Controller;

use App\Repository\ParkingSpotRepository;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

class DashboardController extends AbstractController
{
    #[Route('/', name: 'app_home')]
    #[Route('/dashboard', name: 'app_dashboard')]
    public function index(ParkingSpotRepository $parkingSpotRepository): Response
    {
        $spots = $parkingSpotRepository->findAll();

        return $this->render('dashboard/index.html.twig', [
            'spots' => $spots,
        ]);
    }
}
