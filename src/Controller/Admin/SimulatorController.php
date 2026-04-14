<?php

namespace App\Controller\Admin;

use App\Repository\ParkingSpotRepository;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/admin/simulator', name: 'admin_simulator_')]
#[IsGranted('ROLE_ADMIN')]
class SimulatorController extends AbstractController
{
    #[Route('/', name: 'index')]
    public function index(ParkingSpotRepository $spotRepo): Response
    {
        return $this->render('admin/simulator.html.twig', [
            'spots' => $spotRepo->findAll(),
        ]);
    }
}
