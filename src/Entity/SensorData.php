<?php

namespace App\Entity;

use App\Repository\SensorDataRepository;
use Doctrine\ORM\Mapping as ORM;

#[ORM\Entity(repositoryClass: SensorDataRepository::class)]
class SensorData
{
    #[ORM\Id]
    #[ORM\GeneratedValue]
    #[ORM\Column]
    private ?int $id = null;

    #[ORM\ManyToOne(inversedBy: 'sensorData')]
    #[ORM\JoinColumn(nullable: false)]
    private ?ParkingSpot $parkingSpot = null;

    #[ORM\Column]
    private ?bool $isOccupied = null;

    #[ORM\Column(nullable: true)]
    private ?float $distanceCm = null;

    #[ORM\Column(length: 50, nullable: true)]
    private ?string $esp32Id = null;

    #[ORM\Column]
    private ?\DateTimeImmutable $recordedAt = null;

    public function getId(): ?int
    {
        return $this->id;
    }

    public function getParkingSpot(): ?ParkingSpot
    {
        return $this->parkingSpot;
    }

    public function setParkingSpot(?ParkingSpot $parkingSpot): static
    {
        $this->parkingSpot = $parkingSpot;

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

    public function getDistanceCm(): ?float
    {
        return $this->distanceCm;
    }

    public function setDistanceCm(?float $distanceCm): static
    {
        $this->distanceCm = $distanceCm;

        return $this;
    }

    public function getEsp32Id(): ?string
    {
        return $this->esp32Id;
    }

    public function setEsp32Id(?string $esp32Id): static
    {
        $this->esp32Id = $esp32Id;

        return $this;
    }

    public function getRecordedAt(): ?\DateTimeImmutable
    {
        return $this->recordedAt;
    }

    public function setRecordedAt(\DateTimeImmutable $recordedAt): static
    {
        $this->recordedAt = $recordedAt;

        return $this;
    }
}
