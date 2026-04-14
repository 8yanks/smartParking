<?php

namespace App\Controller;

use App\Entity\Reservation;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/payment')]
#[IsGranted('ROLE_USER')]
class PaymentSimulatorController extends AbstractController
{
    #[Route('/simulate/{id}', name: 'app_payment_simulate')]
    public function index(Reservation $reservation, EntityManagerInterface $em): Response
    {
        // Security check: Must belong to user OR user is admin
        if ($reservation->getUser() !== $this->getUser() && !$this->isGranted('ROLE_ADMIN')) {
            throw $this->createAccessDeniedException('Vous ne pouvez pas payer cette réservation.');
        }

        if ($reservation->getStatus() === 'CONFIRMED') {
            $this->addFlash('info', 'Cette réservation est déjà payée.');
            return $this->redirectToRoute('app_profile');
        }

        return $this->render('payment/simulate.html.twig', [
            'reservation' => $reservation,
        ]);
    }

    #[Route('/process/{id}', name: 'app_payment_process', methods: ['POST'])]
    public function process(Reservation $reservation, EntityManagerInterface $em): Response
    {
        if ($reservation->getUser() !== $this->getUser() && !$this->isGranted('ROLE_ADMIN')) {
            throw $this->createAccessDeniedException();
        }

        // Simulate payment success
        $reservation->setStatus('CONFIRMED');
        $em->flush();

        $this->addFlash('success', 'Paiement de ' . $reservation->getTotalPrice() . '€ validé avec succès ! Votre place est réservée.');
        return $this->redirectToRoute('app_profile');
    }
}
