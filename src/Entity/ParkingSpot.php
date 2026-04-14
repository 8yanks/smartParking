<?php

namespace App\Entity;

use App\Repository\ParkingSpotRepository;
use Doctrine\Common\Collections\ArrayCollection;
use Doctrine\Common\Collections\Collection;
use Doctrine\ORM\Mapping as ORM;

#[ORM\Entity(repositoryClass: ParkingSpotRepository::class)]
class ParkingSpot
{
    #[ORM\Id]
    #[ORM\GeneratedValue]
    #[ORM\Column]
    private ?int $id = null;

    #[ORM\Column(length: 50)]
    private ?string $name = null;

    #[ORM\Column]
    private ?bool $isOccupied = null;

    #[ORM\Column(nullable: true)]
    private ?\DateTimeImmutable $lastUpdatedAt = null;

    #[ORM\Column]
    private ?bool $isActive = null;

    #[ORM\OneToMany(mappedBy: 'parkingSpot', targetEntity: SensorData::class)]
    private Collection $sensorData;

    #[ORM\OneToMany(mappedBy: 'parkingSpot', targetEntity: Reservation::class)]
    private Collection $reservations;

    public function __construct()
    {
        $this->sensorData = new ArrayCollection();
        $this->reservations = new ArrayCollection();
    }

    public function getId(): ?int
    {
        return $this->id;
    }

    public function getName(): ?string
    {
        return $this->name;
    }

    public function setName(string $name): static
    {
        $this->name = $name;

        return $this;
    }

    public function isOccupied(): ?bool
    {
        return $this->isOccupied;
    }

    public function setIsOccupied(bool $isOccupied): static
    {
        $this->isOccupied = $isOccupied;

        return $this;
    }

    public function getLastUpdatedAt(): ?\DateTimeImmutable
    {
        return $this->lastUpdatedAt;
    }

    public function setLastUpdatedAt(?\DateTimeImmutable $lastUpdatedAt): static
    {
        $this->lastUpdatedAt = $lastUpdatedAt;

        return $this;
    }

    public function isActive(): ?bool
    {
        return $this->isActive;
    }

    public function setIsActive(bool $isActive): static
    {
        $this->isActive = $isActive;

        return $this;
    }

    /**
     * @return Collection<int, SensorData>
     */
    public function getSensorData(): Collection
    {
        return $this->sensorData;
    }

    public function addSensorData(SensorData $sensorData): static
    {
        if (!$this->sensorData->contains($sensorData)) {
            $this->sensorData->add($sensorData);
            $sensorData->setParkingSpot($this);
        }

        return $this;
    }

    public function removeSensorData(SensorData $sensorData): static
    {
        if ($this->sensorData->removeElement($sensorData)) {
            if ($sensorData->getParkingSpot() === $this) {
                $sensorData->setParkingSpot(null);
            }
        }

        return $this;
    }

    /**
     * @return Collection<int, Reservation>
     */
    public function getReservations(): Collection
    {
        return $this->reservations;
    }

    public function addReservation(Reservation $reservation): static
    {
        if (!$this->reservations->contains($reservation)) {
            $this->reservations->add($reservation);
            $reservation->setParkingSpot($this);
        }

        return $this;
    }

    public function removeReservation(Reservation $reservation): static
    {
        if ($this->reservations->removeElement($reservation)) {
            if ($reservation->getParkingSpot() === $this) {
                $reservation->setParkingSpot(null);
            }
        }

        return $this;
    }
}
