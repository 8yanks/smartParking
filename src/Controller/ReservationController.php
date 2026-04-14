<?php

namespace App\Controller;

use App\Entity\Reservation;
use App\Form\ReservationType;
use App\Repository\ReservationRepository;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/reservation')]
#[IsGranted('ROLE_USER')]
class ReservationController extends AbstractController
{
    #[Route('/new', name: 'app_reservation_new')]
    public function new(Request $request, ReservationRepository $reservationRepo, EntityManagerInterface $em): Response
    {
        $reservation = new Reservation();
        $form = $this->createForm(ReservationType::class, $reservation);
        $form->handleRequest($request);

        if ($form->isSubmitted() && $form->isValid()) {
            if ($reservation->getEndTime() <= $reservation->getStartTime()) {
                $this->addFlash('error', 'La date de fin doit être après la date de début.');
                return $this->render('reservation/new.html.twig', [
                    'form' => $form,
                ]);
            }

            // Vérification des chevauchements
            $conflicts = $reservationRepo->findOverlappingReservations(
                $reservation->getParkingSpot()->getId(),
                $reservation->getStartTime(),
                $reservation->getEndTime()
            );

            if (count($conflicts) > 0) {
                $this->addFlash('error', 'Cette place est malheureusement déjà réservée sur ce créneau.');
                return $this->render('reservation/new.html.twig', [
                    'form' => $form,
                ]);
            }

            // Calcul du prix (ex: 2€ / heure)
            $diff = $reservation->getStartTime()->diff($reservation->getEndTime());
            $hours = $diff->h + ($diff->days * 24);
            $minutes = $diff->i;
            $totalHours = $hours + ($minutes / 60);
            $price = max(1, $totalHours * 2.0); // Minimum 1€

            $reservation->setUser($this->getUser());
            $reservation->setStatus('PENDING'); // En attente de paiement
            $reservation->setTotalPrice((string) round($price, 2));
            $reservation->setCreatedAt(new \DateTimeImmutable());

            $em->persist($reservation);
            $em->flush();

            return $this->redirectToRoute('app_payment_simulate', ['id' => $reservation->getId()]);
        }

        return $this->render('reservation/new.html.twig', [
            'form' => $form,
        ]);
    }

    #[Route('/cancel/{id}', name: 'app_reservation_cancel')]
    public function cancel(Reservation $reservation, EntityManagerInterface $em): Response
    {
        if ($reservation->getUser() !== $this->getUser() && !$this->isGranted('ROLE_ADMIN')) {
            throw $this->createAccessDeniedException('Vous ne pouvez pas annuler cette réservation.');
        }

        // Vérifier si la réservation est à plus de 7 jours
        $now = new \DateTimeImmutable();
        $interval = $now->diff($reservation->getStartTime());
        
        $daysUntilStart = $interval->days;
        if ($interval->invert === 1) { // La date de début est dans le passé
            $daysUntilStart = -1;
        }

        if ($daysUntilStart >= 7) {
            $reservation->setStatus('CANCELLED');
            $em->flush();
            $this->addFlash('success', 'Votre réservation a bien été annulée car vous avez prévenu plus d\'une semaine à l\'avance.');
        } else {
            $this->addFlash('error', 'Il est trop tard pour annuler cette réservation. Les annulations ne sont possibles que jusqu\'à 7 jours à l\'avance.');
        }

        return $this->redirectToRoute('app_profile');
    }
}
