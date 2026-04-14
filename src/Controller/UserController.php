<?php

namespace App\Controller;

use App\Repository\ReservationRepository;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/profile')]
#[IsGranted('ROLE_USER')]
class UserController extends AbstractController
{
    #[Route('/', name: 'app_profile')]
    public function index(ReservationRepository $reservationRepo): Response
    {
        $user = $this->getUser();
        $reservations = $reservationRepo->findBy(
            ['user' => $user],
            ['createdAt' => 'DESC']
        );

        return $this->render('user/profile.html.twig', [
            'reservations' => $reservations,
        ]);
    }
}
