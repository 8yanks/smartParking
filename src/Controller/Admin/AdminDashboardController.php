<?php

namespace App\Controller\Admin;

use App\Repository\ParkingSpotRepository;
use App\Repository\UserRepository;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/admin', name: 'admin_')]
#[IsGranted('ROLE_ADMIN')]
class AdminDashboardController extends AbstractController
{
    #[Route('/', name: 'dashboard')]
    public function index(ParkingSpotRepository $spotRepo, UserRepository $userRepo): Response
    {
        $spots = $spotRepo->findAll();
        $users = $userRepo->findAll();

        $occupiedCount = 0;
        foreach ($spots as $spot) {
            if ($spot->isOccupied()) {
                $occupiedCount++;
            }
        }

        return $this->render('admin/dashboard.html.twig', [
            'total_spots' => count($spots),
            'occupied_spots' => $occupiedCount,
            'total_users' => count($users),
            'spots' => $spots
        ]);
    }
}
