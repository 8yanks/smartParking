<?php

namespace App\Repository;

use App\Entity\Reservation;
use Doctrine\Bundle\DoctrineBundle\Repository\ServiceEntityRepository;
use Doctrine\Persistence\ManagerRegistry;

/**
 * @extends ServiceEntityRepository<Reservation>
 */
class ReservationRepository extends ServiceEntityRepository
{
    public function __construct(ManagerRegistry $registry)
    {
        parent::__construct($registry, Reservation::class);
    }

    /**
     * Retourne les réservations qui chevauchent les dates données pour une place spécifique.
     * On ne considère que les réservations CONFIRMED ou PENDING.
     */
    public function findOverlappingReservations(int $spotId, \DateTimeImmutable $start, \DateTimeImmutable $end): array
    {
        return $this->createQueryBuilder('r')
            ->andWhere('r.parkingSpot = :spotId')
            ->setParameter('spotId', $spotId)
            ->andWhere('r.status IN (:statuses)')
            ->setParameter('statuses', ['PENDING', 'CONFIRMED'])
            // Une réservation chevauche si (nouvelle_fin > ancienne_debut) ET (nouveau_debut < ancienne_fin)
            ->andWhere('r.startTime < :end')
            ->andWhere('r.endTime > :start')
            ->setParameter('start', $start)
            ->setParameter('end', $end)
            ->getQuery()
            ->getResult();
    }
}
