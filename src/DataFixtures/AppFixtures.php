<?php

namespace App\DataFixtures;

use App\Entity\ParkingSpot;
use App\Entity\User;
use Doctrine\Bundle\FixturesBundle\Fixture;
use Doctrine\Persistence\ObjectManager;
use Symfony\Component\PasswordHasher\Hasher\UserPasswordHasherInterface;

class AppFixtures extends Fixture
{
    public function __construct(
        private UserPasswordHasherInterface $hasher
    ) {}

    public function load(ObjectManager $manager): void
    {
        // Create 6 parking spots
        $spotNames = ['A1', 'A2', 'A3', 'B1', 'B2', 'B3'];
        foreach ($spotNames as $name) {
            $spot = new ParkingSpot();
            $spot->setName('Place ' . $name);
            $spot->setIsOccupied(false);
            $spot->setIsActive(true);
            $manager->persist($spot);
        }

        // Create admin account
        $admin = new User();
        $admin->setEmail('admin@parking.fr');
        $admin->setFirstName('Admin');
        $admin->setLastName('Parking');
        $admin->setRoles(['ROLE_ADMIN']);
        $admin->setCreatedAt(new \DateTimeImmutable());
        $admin->setRgpdConsentAt(new \DateTimeImmutable());
        $admin->setPassword($this->hasher->hashPassword($admin, 'admin1234'));
        $manager->persist($admin);

        // Create test user account
        $user = new User();
        $user->setEmail('user@parking.fr');
        $user->setFirstName('Jean');
        $user->setLastName('Dupont');
        $user->setRoles(['ROLE_USER']);
        $user->setCreatedAt(new \DateTimeImmutable());
        $user->setRgpdConsentAt(new \DateTimeImmutable());
        $user->setPassword($this->hasher->hashPassword($user, 'user1234'));
        $manager->persist($user);

        $manager->flush();
    }
}
