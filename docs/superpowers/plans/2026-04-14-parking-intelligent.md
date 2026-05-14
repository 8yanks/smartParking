# Parking Intelligent — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Construire un site web Symfony 7 complet pour un parking intelligent de 6 places avec API REST pour ESP32, dashboard temps réel, réservation, paiement Stripe et abonnements.

**Architecture:** Symfony 7 (PHP 8.3) + MySQL 8 + Doctrine ORM. Les ESP32 communiquent via `POST /api/spot/{id}` avec une clé API. Le dashboard se rafraîchit via polling JS toutes les 5 secondes sur `GET /api/spots`.

**Tech Stack:** Symfony 7, PHP 8.3, MySQL 8, Doctrine ORM, Twig, Symfony Security, Stripe PHP SDK, PHPUnit, Symfony CLI

---

## Prérequis à installer avant de commencer

Avant toute chose, vérifiez que ces outils sont installés sur votre machine :

1. **PHP 8.3** — [php.net/downloads](https://www.php.net/downloads)
2. **Composer** — [getcomposer.org](https://getcomposer.org)
3. **Symfony CLI** — [symfony.com/download](https://symfony.com/download)
4. **MySQL 8** — via XAMPP ou installation directe
5. **Git** — [git-scm.com](https://git-scm.com)

Pour vérifier : `php -v`, `composer -V`, `symfony version`

---

## Structure des fichiers

```
siteReservation/
├── config/
│   ├── packages/
│   │   ├── security.yaml          ← firewall, roles, encoders
│   │   └── rate_limiter.yaml      ← limite requêtes API ESP32
│   └── services.yaml              ← injection de dépendances
├── src/
│   ├── Controller/
│   │   ├── HomeController.php         ← page d'accueil
│   │   ├── DashboardController.php    ← vue temps réel
│   │   ├── ReservationController.php  ← CRUD réservations
│   │   ├── SubscriptionController.php ← gestion abonnements
│   │   ├── PaymentController.php      ← paiement Stripe
│   │   ├── ProfileController.php      ← profil utilisateur
│   │   ├── Admin/
│   │   │   └── AdminController.php    ← back-office
│   │   └── Api/
│   │       └── SpotApiController.php  ← endpoints pour ESP32
│   ├── Entity/
│   │   ├── User.php
│   │   ├── ParkingSpot.php
│   │   ├── SensorData.php
│   │   ├── Reservation.php
│   │   ├── Subscription.php
│   │   └── Payment.php
│   ├── Repository/              ← une classe par entité (générées auto)
│   ├── Form/
│   │   ├── RegistrationFormType.php
│   │   ├── ReservationFormType.php
│   │   └── SubscriptionFormType.php
│   ├── Service/
│   │   ├── ParkingService.php         ← logique métier des places
│   │   ├── ReservationService.php     ← logique réservation
│   │   ├── SubscriptionService.php    ← logique abonnement
│   │   └── StripeService.php          ← appels API Stripe
│   ├── Security/
│   │   └── ApiKeyAuthenticator.php    ← auth des ESP32
│   └── DataFixtures/
│       └── AppFixtures.php            ← données initiales (6 places + admin)
├── templates/
│   ├── base.html.twig
│   ├── home/index.html.twig
│   ├── dashboard/index.html.twig
│   ├── reservation/
│   │   ├── index.html.twig
│   │   └── show.html.twig
│   ├── subscription/index.html.twig
│   ├── payment/index.html.twig
│   ├── profile/index.html.twig
│   └── admin/
│       ├── index.html.twig
│       ├── spots.html.twig
│       ├── reservations.html.twig
│       └── users.html.twig
├── public/
│   ├── css/app.css
│   └── js/dashboard.js            ← polling toutes les 5s
├── tests/
│   ├── Controller/
│   │   ├── DashboardControllerTest.php
│   │   └── SpotApiControllerTest.php
│   └── Service/
│       ├── ParkingServiceTest.php
│       └── ReservationServiceTest.php
├── migrations/
├── .env
└── .env.local                     ← secrets (jamais commité)
```

---

## Task 1 : Installation et configuration du projet Symfony

**Files:**
- Create: `.env.local`
- Modify: `.env`
- Create: `config/packages/rate_limiter.yaml`

- [ ] **Step 1 : Créer le projet Symfony**

Dans votre terminal, placez-vous dans le dossier parent et lancez :
```bash
cd C:/Users/yanis/Documents/Cours/Projet
symfony new siteReservation --version="7.2.*" --webapp
cd siteReservation
```
`--webapp` installe Twig, Doctrine, Form, Security, et d'autres paquets utiles automatiquement.

- [ ] **Step 2 : Installer les dépendances supplémentaires**

```bash
composer require stripe/stripe-php
composer require orm-fixtures --dev
composer require phpunit --dev
composer require symfony/rate-limiter
```

- [ ] **Step 3 : Créer la base de données MySQL**

Ouvrez MySQL (via XAMPP ou en ligne de commande) et créez la base :
```sql
CREATE DATABASE parking_intelligent CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'parking_user'@'localhost' IDENTIFIED BY 'parking_password';
GRANT ALL PRIVILEGES ON parking_intelligent.* TO 'parking_user'@'localhost';
FLUSH PRIVILEGES;
```

- [ ] **Step 4 : Configurer `.env.local`**

Créez le fichier `.env.local` (ce fichier ne sera JAMAIS commité — il contient les secrets) :
```dotenv
DATABASE_URL="mysql://parking_user:parking_password@127.0.0.1:3306/parking_intelligent?serverVersion=8.0"
STRIPE_SECRET_KEY=sk_test_VOTRE_CLE_STRIPE_ICI
STRIPE_PUBLIC_KEY=pk_test_VOTRE_CLE_STRIPE_ICI
PARKING_API_KEY=une_cle_secrete_longue_pour_esp32_changez_moi
APP_SECRET=votre_app_secret_symfony
```

- [ ] **Step 5 : Ajouter `.env.local` au `.gitignore`**

Vérifiez que `.env.local` est bien dans `.gitignore` (il l'est par défaut avec Symfony).
```bash
grep ".env.local" .gitignore
```
Doit afficher `.env.local`.

- [ ] **Step 6 : Configurer le rate limiter pour l'API ESP32**

Créez `config/packages/rate_limiter.yaml` :
```yaml
framework:
    rate_limiter:
        esp32_api:
            policy: 'fixed_window'
            limit: 60
            interval: '1 minute'
```

- [ ] **Step 7 : Commit**

```bash
git add .
git commit -m "feat: init projet Symfony + config DB + rate limiter"
```

---

## Task 2 : Entités Doctrine (base de données)

**Files:**
- Create: `src/Entity/User.php`
- Create: `src/Entity/ParkingSpot.php`
- Create: `src/Entity/SensorData.php`
- Create: `src/Entity/Reservation.php`
- Create: `src/Entity/Subscription.php`
- Create: `src/Entity/Payment.php`
- Create: `migrations/` (générées automatiquement)

- [ ] **Step 1 : Créer l'entité User**

```bash
php bin/console make:user
```
Répondez aux questions :
- Name of security user class: `User`
- Do you want to store user data in the database (via Doctrine)? `yes`
- Enter a property name that will be the unique "display" name: `email`
- Does this app need to hash/check user passwords? `yes`

Ensuite ajoutez les propriétés manquantes :
```bash
php bin/console make:entity User
```
Ajoutez : `firstName` (string, 100, not null), `lastName` (string, 100, not null), `createdAt` (datetime_immutable, not null), `rgpdConsentAt` (datetime_immutable, nullable).

- [ ] **Step 2 : Créer l'entité ParkingSpot**

```bash
php bin/console make:entity ParkingSpot
```
Ajoutez ces propriétés :
- `name` → type: string, longueur: 50, not nullable (ex: "Place A1")
- `isOccupied` → type: boolean, not nullable (défaut: false)
- `lastUpdatedAt` → type: datetime_immutable, nullable
- `isActive` → type: boolean, not nullable (défaut: true)

- [ ] **Step 3 : Créer l'entité SensorData**

```bash
php bin/console make:entity SensorData
```
Ajoutez :
- `parkingSpot` → type: relation, ManyToOne vers ParkingSpot, not nullable
- `isOccupied` → type: boolean, not nullable
- `distanceCm` → type: float, nullable
- `esp32Id` → type: string, longueur: 50, nullable
- `recordedAt` → type: datetime_immutable, not nullable

- [ ] **Step 4 : Créer l'entité Reservation**

```bash
php bin/console make:entity Reservation
```
Ajoutez :
- `user` → relation ManyToOne vers User, not nullable
- `parkingSpot` → relation ManyToOne vers ParkingSpot, not nullable
- `startTime` → type: datetime_immutable, not nullable
- `endTime` → type: datetime_immutable, not nullable
- `status` → type: string, longueur: 20, not nullable (valeurs: pending, confirmed, cancelled, completed)
- `totalPrice` → type: decimal, precision: 10, scale: 2, not nullable
- `createdAt` → type: datetime_immutable, not nullable

- [ ] **Step 5 : Créer l'entité Subscription**

```bash
php bin/console make:entity Subscription
```
Ajoutez :
- `user` → relation ManyToOne vers User, not nullable
- `plan` → type: string, longueur: 20, not nullable (valeurs: monthly, annual)
- `price` → type: decimal, precision: 10, scale: 2, not nullable
- `startDate` → type: datetime_immutable, not nullable
- `endDate` → type: datetime_immutable, not nullable
- `status` → type: string, longueur: 20, not nullable (valeurs: active, expired, cancelled)
- `stripeSubscriptionId` → type: string, longueur: 255, nullable

- [ ] **Step 6 : Créer l'entité Payment**

```bash
php bin/console make:entity Payment
```
Ajoutez :
- `user` → relation ManyToOne vers User, not nullable
- `reservation` → relation ManyToOne vers Reservation, nullable
- `subscription` → relation ManyToOne vers Subscription, nullable
- `amount` → type: decimal, precision: 10, scale: 2, not nullable
- `currency` → type: string, longueur: 3, not nullable (défaut: "EUR")
- `status` → type: string, longueur: 20, not nullable (valeurs: pending, succeeded, failed)
- `stripePaymentIntentId` → type: string, longueur: 255, nullable
- `createdAt` → type: datetime_immutable, not nullable

- [ ] **Step 7 : Générer et exécuter les migrations**

```bash
php bin/console make:migration
php bin/console doctrine:migrations:migrate
```
Répondez `yes` à la confirmation. Vérifiez dans MySQL que les 6 tables ont été créées :
```bash
php bin/console doctrine:query:sql "SHOW TABLES"
```
Attendu : `payment`, `parking_spot`, `reservation`, `sensor_data`, `subscription`, `user`

- [ ] **Step 8 : Créer les fixtures (données initiales)**

Créez `src/DataFixtures/AppFixtures.php` :
```php
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
        // Créer les 6 places de parking
        $spotNames = ['A1', 'A2', 'A3', 'B1', 'B2', 'B3'];
        foreach ($spotNames as $name) {
            $spot = new ParkingSpot();
            $spot->setName('Place ' . $name);
            $spot->setIsOccupied(false);
            $spot->setIsActive(true);
            $manager->persist($spot);
        }

        // Créer un compte admin
        $admin = new User();
        $admin->setEmail('admin@parking.fr');
        $admin->setFirstName('Admin');
        $admin->setLastName('Parking');
        $admin->setRoles(['ROLE_ADMIN']);
        $admin->setCreatedAt(new \DateTimeImmutable());
        $admin->setRgpdConsentAt(new \DateTimeImmutable());
        $admin->setPassword($this->hasher->hashPassword($admin, 'admin1234'));
        $manager->persist($admin);

        // Créer un compte utilisateur de test
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
```

- [ ] **Step 9 : Charger les fixtures**

```bash
php bin/console doctrine:fixtures:load
```
Répondez `yes`. Les 6 places et les 2 comptes de test sont créés.

- [ ] **Step 10 : Commit**

```bash
git add .
git commit -m "feat: entités Doctrine + migrations + fixtures (6 places, admin, user test)"
```

---

## Task 3 : Authentification (inscription et connexion)

**Files:**
- Modify: `config/packages/security.yaml`
- Create: `src/Controller/HomeController.php`
- Create: `src/Form/RegistrationFormType.php`
- Create: `templates/base.html.twig`
- Create: `templates/home/index.html.twig`
- Create: `templates/registration/register.html.twig`
- Create: `templates/security/login.html.twig`

- [ ] **Step 1 : Générer le système d'authentification**

```bash
php bin/console make:auth
```
Répondez :
- What style of authentication? `1` (Login form authenticator)
- The class name of authenticator: `AppAuthenticator`
- Controller class: `SecurityController`
- Generate a '/logout' URL? `yes`

- [ ] **Step 2 : Générer le formulaire d'inscription**

```bash
php bin/console make:registration-form
```
Répondez :
- Do you want to add a @UniqueEntity validation? `yes`
- Do you want to send an email to verify the user's email address? `no` (trop complexe pour l'instant)
- Do you want to automatically authenticate the user after registration? `yes`
- What route should the user be redirected to after registration? `app_dashboard`

- [ ] **Step 3 : Configurer `config/packages/security.yaml`**

Remplacez le contenu par :
```yaml
security:
    password_hashers:
        Symfony\Component\Security\Core\User\PasswordAuthenticatedUserInterface: 'auto'

    providers:
        app_user_provider:
            entity:
                class: App\Entity\User
                property: email

    firewalls:
        dev:
            pattern: ^/(_(profiler|wdt)|css|images|js)/
            security: false

        api:
            pattern: ^/api/spot
            stateless: true
            custom_authenticators:
                - App\Security\ApiKeyAuthenticator

        main:
            lazy: true
            provider: app_user_provider
            form_login:
                login_path: app_login
                check_path: app_login
                enable_csrf: true
                default_target_path: app_dashboard
            logout:
                path: app_logout
                target: app_home
            remember_me:
                secret: '%kernel.secret%'
                lifetime: 604800

    access_control:
        - { path: ^/admin, roles: ROLE_ADMIN }
        - { path: ^/reservation, roles: ROLE_USER }
        - { path: ^/abonnement, roles: ROLE_USER }
        - { path: ^/profil, roles: ROLE_USER }
        - { path: ^/api/spots$, roles: PUBLIC_ACCESS }
        - { path: ^/api/spot/\d+$, roles: PUBLIC_ACCESS }

    role_hierarchy:
        ROLE_ADMIN: ROLE_USER
```

- [ ] **Step 4 : Créer le template de base `templates/base.html.twig`**

```twig
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{% block title %}Parking Intelligent{% endblock %}</title>
    <link rel="stylesheet" href="{{ asset('css/app.css') }}">
    {% block stylesheets %}{% endblock %}
</head>
<body>
    <nav class="navbar">
        <a href="{{ path('app_home') }}" class="navbar-brand">Parking Intelligent</a>
        <ul class="navbar-menu">
            <li><a href="{{ path('app_dashboard') }}">Tableau de bord</a></li>
            {% if is_granted('ROLE_USER') %}
                <li><a href="{{ path('app_reservation') }}">Réserver</a></li>
                <li><a href="{{ path('app_subscription') }}">Abonnement</a></li>
                <li><a href="{{ path('app_profile') }}">Mon profil</a></li>
                {% if is_granted('ROLE_ADMIN') %}
                    <li><a href="{{ path('app_admin') }}">Admin</a></li>
                {% endif %}
                <li><a href="{{ path('app_logout') }}">Déconnexion</a></li>
            {% else %}
                <li><a href="{{ path('app_login') }}">Connexion</a></li>
                <li><a href="{{ path('app_register') }}">Inscription</a></li>
            {% endif %}
        </ul>
    </nav>

    <main class="container">
        {% for message in app.flashes('success') %}
            <div class="alert alert-success">{{ message }}</div>
        {% endfor %}
        {% for message in app.flashes('error') %}
            <div class="alert alert-error">{{ message }}</div>
        {% endfor %}

        {% block body %}{% endblock %}
    </main>

    {% block javascripts %}{% endblock %}
</body>
</html>
```

- [ ] **Step 5 : Créer HomeController**

Créez `src/Controller/HomeController.php` :
```php
<?php

namespace App\Controller;

use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

class HomeController extends AbstractController
{
    #[Route('/', name: 'app_home')]
    public function index(): Response
    {
        return $this->render('home/index.html.twig');
    }
}
```

- [ ] **Step 6 : Créer `templates/home/index.html.twig`**

```twig
{% extends 'base.html.twig' %}

{% block title %}Parking Intelligent — Accueil{% endblock %}

{% block body %}
<section class="hero">
    <h1>Bienvenue au Parking Intelligent</h1>
    <p>6 places connectées. Réservez en temps réel, payez en ligne.</p>
    <div class="hero-actions">
        <a href="{{ path('app_dashboard') }}" class="btn btn-primary">Voir les places disponibles</a>
        {% if not is_granted('ROLE_USER') %}
            <a href="{{ path('app_register') }}" class="btn btn-secondary">Créer un compte</a>
        {% endif %}
    </div>
</section>

<section class="features">
    <div class="feature-card">
        <h3>Temps réel</h3>
        <p>L'état des places est mis à jour automatiquement grâce aux capteurs ultrasoniques.</p>
    </div>
    <div class="feature-card">
        <h3>Réservation simple</h3>
        <p>Réservez votre place à l'avance et payez en ligne en quelques clics.</p>
    </div>
    <div class="feature-card">
        <h3>Abonnement</h3>
        <p>Abonnez-vous pour bénéficier de tarifs préférentiels et d'une réservation prioritaire.</p>
    </div>
</section>
{% endblock %}
```

- [ ] **Step 7 : Démarrer le serveur et tester l'inscription**

```bash
symfony server:start -d
```
Ouvrez `http://127.0.0.1:8000/register` dans votre navigateur.
Créez un compte test. Vérifiez que la redirection vers le dashboard fonctionne (même si la page est vide pour l'instant).

- [ ] **Step 8 : Commit**

```bash
git add .
git commit -m "feat: authentification (inscription, connexion, déconnexion)"
```

---

## Task 4 : Dashboard temps réel + API ESP32 (GET)

**Files:**
- Create: `src/Controller/DashboardController.php`
- Create: `src/Controller/Api/SpotApiController.php`
- Create: `src/Service/ParkingService.php`
- Create: `templates/dashboard/index.html.twig`
- Create: `public/js/dashboard.js`
- Create: `public/css/app.css`
- Test: `tests/Controller/DashboardControllerTest.php`

- [ ] **Step 1 : Écrire le test du dashboard (TDD)**

Créez `tests/Controller/DashboardControllerTest.php` :
```php
<?php

namespace App\Tests\Controller;

use Symfony\Bundle\FrameworkBundle\Test\WebTestCase;

class DashboardControllerTest extends WebTestCase
{
    public function testDashboardIsAccessible(): void
    {
        $client = static::createClient();
        $client->request('GET', '/dashboard');
        $this->assertResponseIsSuccessful();
    }

    public function testApiSpotsReturnsJson(): void
    {
        $client = static::createClient();
        $client->request('GET', '/api/spots');
        $this->assertResponseIsSuccessful();
        $this->assertResponseHeaderSame('content-type', 'application/json');
        $data = json_decode($client->getResponse()->getContent(), true);
        $this->assertIsArray($data);
        $this->assertCount(6, $data);
    }
}
```

- [ ] **Step 2 : Lancer les tests pour vérifier qu'ils échouent**

```bash
php bin/phpunit tests/Controller/DashboardControllerTest.php
```
Attendu : FAIL (les routes n'existent pas encore).

- [ ] **Step 3 : Créer ParkingService**

Créez `src/Service/ParkingService.php` :
```php
<?php

namespace App\Service;

use App\Entity\ParkingSpot;
use App\Repository\ParkingSpotRepository;

class ParkingService
{
    public function __construct(
        private ParkingSpotRepository $spotRepository
    ) {}

    /** @return ParkingSpot[] */
    public function getAllSpots(): array
    {
        return $this->spotRepository->findBy(['isActive' => true], ['id' => 'ASC']);
    }

    public function countFreeSpots(): int
    {
        return $this->spotRepository->count(['isOccupied' => false, 'isActive' => true]);
    }

    public function countOccupiedSpots(): int
    {
        return $this->spotRepository->count(['isOccupied' => true, 'isActive' => true]);
    }
}
```

- [ ] **Step 4 : Créer DashboardController**

Créez `src/Controller/DashboardController.php` :
```php
<?php

namespace App\Controller;

use App\Service\ParkingService;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

class DashboardController extends AbstractController
{
    #[Route('/dashboard', name: 'app_dashboard')]
    public function index(ParkingService $parkingService): Response
    {
        return $this->render('dashboard/index.html.twig', [
            'spots' => $parkingService->getAllSpots(),
            'freeCount' => $parkingService->countFreeSpots(),
            'occupiedCount' => $parkingService->countOccupiedSpots(),
        ]);
    }
}
```

- [ ] **Step 5 : Créer SpotApiController (GET)**

Créez `src/Controller/Api/SpotApiController.php` :
```php
<?php

namespace App\Controller\Api;

use App\Service\ParkingService;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/api')]
class SpotApiController extends AbstractController
{
    #[Route('/spots', name: 'api_spots', methods: ['GET'])]
    public function getSpots(ParkingService $parkingService): JsonResponse
    {
        $spots = $parkingService->getAllSpots();
        $data = array_map(fn($spot) => [
            'id' => $spot->getId(),
            'name' => $spot->getName(),
            'isOccupied' => $spot->isIsOccupied(),
            'lastUpdatedAt' => $spot->getLastUpdatedAt()?->format('Y-m-d H:i:s'),
        ], $spots);

        return $this->json($data);
    }
}
```

- [ ] **Step 6 : Créer le template du dashboard `templates/dashboard/index.html.twig`**

```twig
{% extends 'base.html.twig' %}

{% block title %}Tableau de bord — Parking{% endblock %}

{% block body %}
<div class="dashboard-header">
    <h1>État du parking en temps réel</h1>
    <div class="stats">
        <span class="stat free">{{ freeCount }} libre{{ freeCount > 1 ? 's' : '' }}</span>
        <span class="stat occupied">{{ occupiedCount }} occupée{{ occupiedCount > 1 ? 's' : '' }}</span>
    </div>
    <p class="refresh-info">Mise à jour automatique toutes les 5 secondes</p>
</div>

<div class="spots-grid" id="spots-grid">
    {% for spot in spots %}
    <div class="spot-card {{ spot.isIsOccupied ? 'occupied' : 'free' }}" data-spot-id="{{ spot.id }}">
        <div class="spot-icon">{{ spot.isIsOccupied ? '🚗' : '✅' }}</div>
        <h3>{{ spot.name }}</h3>
        <p class="spot-status">{{ spot.isIsOccupied ? 'Occupée' : 'Libre' }}</p>
        {% if not spot.isIsOccupied and is_granted('ROLE_USER') %}
            <a href="{{ path('app_reservation', {spotId: spot.id}) }}" class="btn btn-sm">Réserver</a>
        {% endif %}
    </div>
    {% endfor %}
</div>
{% endblock %}

{% block javascripts %}
<script src="{{ asset('js/dashboard.js') }}"></script>
{% endblock %}
```

- [ ] **Step 7 : Créer `public/js/dashboard.js` (polling toutes les 5s)**

```javascript
// Rafraîchit l'état des places toutes les 5 secondes
function refreshSpots() {
    fetch('/api/spots')
        .then(response => response.json())
        .then(spots => {
            spots.forEach(spot => {
                const card = document.querySelector(`[data-spot-id="${spot.id}"]`);
                if (!card) return;

                card.className = `spot-card ${spot.isOccupied ? 'occupied' : 'free'}`;
                card.querySelector('.spot-icon').textContent = spot.isOccupied ? '🚗' : '✅';
                card.querySelector('.spot-status').textContent = spot.isOccupied ? 'Occupée' : 'Libre';
            });
        })
        .catch(err => console.error('Erreur rafraîchissement:', err));
}

// Lance le polling toutes les 5 secondes
setInterval(refreshSpots, 5000);
```

- [ ] **Step 8 : Créer `public/css/app.css`**

```css
* { box-sizing: border-box; margin: 0; padding: 0; }

body {
    font-family: 'Segoe UI', sans-serif;
    background: #f4f6f9;
    color: #333;
}

.navbar {
    background: #1a1a2e;
    padding: 1rem 2rem;
    display: flex;
    align-items: center;
    justify-content: space-between;
}

.navbar-brand {
    color: #e94560;
    font-size: 1.3rem;
    font-weight: bold;
    text-decoration: none;
}

.navbar-menu {
    list-style: none;
    display: flex;
    gap: 1.5rem;
}

.navbar-menu a {
    color: #ccc;
    text-decoration: none;
    transition: color 0.2s;
}

.navbar-menu a:hover { color: #e94560; }

.container { max-width: 1100px; margin: 2rem auto; padding: 0 1rem; }

.hero {
    text-align: center;
    padding: 4rem 2rem;
    background: linear-gradient(135deg, #1a1a2e, #16213e);
    border-radius: 12px;
    color: white;
    margin-bottom: 2rem;
}

.hero h1 { font-size: 2.5rem; margin-bottom: 1rem; }
.hero p { font-size: 1.2rem; color: #aaa; margin-bottom: 2rem; }

.hero-actions { display: flex; gap: 1rem; justify-content: center; }

.btn {
    padding: 0.75rem 1.5rem;
    border-radius: 8px;
    text-decoration: none;
    font-weight: 600;
    transition: opacity 0.2s;
    display: inline-block;
    border: none;
    cursor: pointer;
    font-size: 1rem;
}

.btn:hover { opacity: 0.85; }
.btn-primary { background: #e94560; color: white; }
.btn-secondary { background: #fff; color: #1a1a2e; }
.btn-sm { padding: 0.4rem 0.8rem; font-size: 0.85rem; }

.features {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 1.5rem;
    margin-bottom: 2rem;
}

.feature-card {
    background: white;
    border-radius: 12px;
    padding: 2rem;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
}

.feature-card h3 { margin-bottom: 0.5rem; color: #1a1a2e; }

/* Dashboard */
.dashboard-header {
    text-align: center;
    margin-bottom: 2rem;
}

.stats {
    display: flex;
    justify-content: center;
    gap: 2rem;
    margin: 1rem 0;
}

.stat {
    font-size: 1.5rem;
    font-weight: bold;
    padding: 0.5rem 1.5rem;
    border-radius: 20px;
}

.stat.free { background: #d4edda; color: #155724; }
.stat.occupied { background: #f8d7da; color: #721c24; }
.refresh-info { color: #888; font-size: 0.85rem; }

.spots-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 1.5rem;
}

.spot-card {
    background: white;
    border-radius: 12px;
    padding: 2rem;
    text-align: center;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    border-left: 6px solid;
    transition: transform 0.2s;
}

.spot-card:hover { transform: translateY(-3px); }
.spot-card.free { border-color: #28a745; }
.spot-card.occupied { border-color: #dc3545; }

.spot-icon { font-size: 3rem; margin-bottom: 0.5rem; }
.spot-card h3 { font-size: 1.2rem; }
.spot-status { color: #666; margin: 0.5rem 0; }

/* Formulaires */
.form-group { margin-bottom: 1rem; }
.form-group label { display: block; margin-bottom: 0.3rem; font-weight: 500; }
.form-group input, .form-group select {
    width: 100%;
    padding: 0.75rem;
    border: 1px solid #ddd;
    border-radius: 8px;
    font-size: 1rem;
}

.card {
    background: white;
    border-radius: 12px;
    padding: 2rem;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    max-width: 600px;
    margin: 0 auto;
}

.alert {
    padding: 1rem 1.5rem;
    border-radius: 8px;
    margin-bottom: 1rem;
}

.alert-success { background: #d4edda; color: #155724; }
.alert-error { background: #f8d7da; color: #721c24; }

/* Tables admin */
table { width: 100%; border-collapse: collapse; }
th, td { padding: 0.75rem; text-align: left; border-bottom: 1px solid #eee; }
th { background: #f8f9fa; font-weight: 600; }

.badge {
    padding: 0.25rem 0.6rem;
    border-radius: 12px;
    font-size: 0.8rem;
    font-weight: 600;
}

.badge-free { background: #d4edda; color: #155724; }
.badge-occupied { background: #f8d7da; color: #721c24; }
.badge-confirmed { background: #cce5ff; color: #004085; }
.badge-pending { background: #fff3cd; color: #856404; }
.badge-cancelled { background: #e2e3e5; color: #383d41; }
.badge-active { background: #d4edda; color: #155724; }
```

- [ ] **Step 9 : Lancer les tests (doivent passer)**

```bash
php bin/phpunit tests/Controller/DashboardControllerTest.php
```
Attendu : 2 tests passent (OK).

- [ ] **Step 10 : Vérifier dans le navigateur**

```
http://127.0.0.1:8000/dashboard
http://127.0.0.1:8000/api/spots
```
Le dashboard doit afficher 6 cartes de places. L'API doit retourner du JSON avec 6 éléments.

- [ ] **Step 11 : Commit**

```bash
git add .
git commit -m "feat: dashboard temps réel + API GET /api/spots + CSS"
```

---

## Task 5 : API ESP32 (POST) — Réception des données capteurs

**Files:**
- Modify: `src/Controller/Api/SpotApiController.php`
- Create: `src/Security/ApiKeyAuthenticator.php`
- Test: `tests/Controller/SpotApiControllerTest.php`

- [ ] **Step 1 : Écrire les tests (TDD)**

Créez `tests/Controller/SpotApiControllerTest.php` :
```php
<?php

namespace App\Tests\Controller;

use Symfony\Bundle\FrameworkBundle\Test\WebTestCase;

class SpotApiControllerTest extends WebTestCase
{
    public function testPostSpotWithoutApiKeyIsRejected(): void
    {
        $client = static::createClient();
        $client->request('POST', '/api/spot/1', [], [], ['CONTENT_TYPE' => 'application/json'], '{"occupied": true}');
        $this->assertResponseStatusCodeSame(401);
    }

    public function testPostSpotWithValidApiKeyUpdatesSpot(): void
    {
        $client = static::createClient();
        $client->request(
            'POST',
            '/api/spot/1',
            [],
            [],
            [
                'CONTENT_TYPE' => 'application/json',
                'HTTP_X_API_KEY' => $_ENV['PARKING_API_KEY'] ?? 'test_api_key',
            ],
            json_encode(['occupied' => true, 'distance_cm' => 8.5, 'esp32_id' => 'node-1'])
        );
        $this->assertResponseIsSuccessful();
        $data = json_decode($client->getResponse()->getContent(), true);
        $this->assertEquals('ok', $data['status']);
    }
}
```

- [ ] **Step 2 : Lancer les tests pour vérifier qu'ils échouent**

```bash
php bin/phpunit tests/Controller/SpotApiControllerTest.php
```
Attendu : FAIL.

- [ ] **Step 3 : Créer ApiKeyAuthenticator**

Créez `src/Security/ApiKeyAuthenticator.php` :
```php
<?php

namespace App\Security;

use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Security\Core\Authentication\Token\TokenInterface;
use Symfony\Component\Security\Core\Exception\AuthenticationException;
use Symfony\Component\Security\Core\Exception\CustomUserMessageAuthenticationException;
use Symfony\Component\Security\Http\Authenticator\AbstractAuthenticator;
use Symfony\Component\Security\Http\Authenticator\Passport\Badge\UserBadge;
use Symfony\Component\Security\Http\Authenticator\Passport\Passport;
use Symfony\Component\Security\Http\Authenticator\Passport\SelfValidatingPassport;

class ApiKeyAuthenticator extends AbstractAuthenticator
{
    public function __construct(private string $parkingApiKey)
    {}

    public function supports(Request $request): ?bool
    {
        return $request->headers->has('X-API-Key');
    }

    public function authenticate(Request $request): Passport
    {
        $apiKey = $request->headers->get('X-API-Key');
        if ($apiKey !== $this->parkingApiKey) {
            throw new CustomUserMessageAuthenticationException('Clé API invalide');
        }

        // On crée un utilisateur fictif "esp32" pour satisfaire Symfony Security
        return new SelfValidatingPassport(new UserBadge('esp32_device'));
    }

    public function onAuthenticationSuccess(Request $request, TokenInterface $token, string $firewallName): ?Response
    {
        return null; // Continue vers le controller
    }

    public function onAuthenticationFailure(Request $request, AuthenticationException $exception): ?Response
    {
        return new JsonResponse(['error' => 'Clé API invalide'], Response::HTTP_UNAUTHORIZED);
    }
}
```

- [ ] **Step 4 : Déclarer la clé API dans `config/services.yaml`**

Ajoutez dans la section `services:` :
```yaml
    App\Security\ApiKeyAuthenticator:
        arguments:
            $parkingApiKey: '%env(PARKING_API_KEY)%'
```

- [ ] **Step 5 : Ajouter la route POST dans SpotApiController**

Modifiez `src/Controller/Api/SpotApiController.php` — ajoutez ces imports et cette méthode :
```php
<?php

namespace App\Controller\Api;

use App\Entity\SensorData;
use App\Repository\ParkingSpotRepository;
use App\Service\ParkingService;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\RateLimiter\RateLimiterFactory;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/api')]
class SpotApiController extends AbstractController
{
    #[Route('/spots', name: 'api_spots', methods: ['GET'])]
    public function getSpots(ParkingService $parkingService): JsonResponse
    {
        $spots = $parkingService->getAllSpots();
        $data = array_map(fn($spot) => [
            'id' => $spot->getId(),
            'name' => $spot->getName(),
            'isOccupied' => $spot->isIsOccupied(),
            'lastUpdatedAt' => $spot->getLastUpdatedAt()?->format('Y-m-d H:i:s'),
        ], $spots);

        return $this->json($data);
    }

    #[Route('/spot/{id}', name: 'api_spot_update', methods: ['POST'], requirements: ['id' => '\d+'])]
    public function updateSpot(
        int $id,
        Request $request,
        ParkingSpotRepository $spotRepository,
        EntityManagerInterface $em,
        RateLimiterFactory $esp32ApiLimiter,
    ): JsonResponse {
        // Rate limiting : max 60 requêtes/minute par IP
        $limiter = $esp32ApiLimiter->create($request->getClientIp());
        if (!$limiter->consume(1)->isAccepted()) {
            return $this->json(['error' => 'Trop de requêtes'], Response::HTTP_TOO_MANY_REQUESTS);
        }

        $spot = $spotRepository->find($id);
        if (!$spot) {
            return $this->json(['error' => 'Place introuvable'], Response::HTTP_NOT_FOUND);
        }

        $payload = json_decode($request->getContent(), true);
        if (!isset($payload['occupied'])) {
            return $this->json(['error' => 'Champ "occupied" manquant'], Response::HTTP_BAD_REQUEST);
        }

        // Mise à jour de la place
        $spot->setIsOccupied((bool) $payload['occupied']);
        $spot->setLastUpdatedAt(new \DateTimeImmutable());

        // Enregistrement historique capteur
        $sensorData = new SensorData();
        $sensorData->setParkingSpot($spot);
        $sensorData->setIsOccupied((bool) $payload['occupied']);
        $sensorData->setDistanceCm($payload['distance_cm'] ?? null);
        $sensorData->setEsp32Id($payload['esp32_id'] ?? null);
        $sensorData->setRecordedAt(new \DateTimeImmutable());

        $em->persist($sensorData);
        $em->flush();

        return $this->json(['status' => 'ok', 'spot_id' => $id]);
    }
}
```

- [ ] **Step 6 : Lancer les tests**

```bash
php bin/phpunit tests/Controller/SpotApiControllerTest.php
```
Attendu : 2 tests passent.

- [ ] **Step 7 : Tester manuellement avec curl (simuler un ESP32)**

```bash
curl -X POST http://127.0.0.1:8000/api/spot/1 \
  -H "Content-Type: application/json" \
  -H "X-API-Key: une_cle_secrete_longue_pour_esp32_changez_moi" \
  -d '{"occupied": true, "distance_cm": 8.5, "esp32_id": "node-1"}'
```
Attendu : `{"status":"ok","spot_id":1}`
Vérifiez ensuite `http://127.0.0.1:8000/dashboard` — la place 1 doit être en rouge.

- [ ] **Step 8 : Commit**

```bash
git add .
git commit -m "feat: API POST /api/spot/{id} + ApiKeyAuthenticator + rate limiting"
```

---

## Task 6 : Système de réservation

**Files:**
- Create: `src/Controller/ReservationController.php`
- Create: `src/Service/ReservationService.php`
- Create: `src/Form/ReservationFormType.php`
- Create: `templates/reservation/index.html.twig`
- Create: `templates/reservation/show.html.twig`
- Test: `tests/Service/ReservationServiceTest.php`

- [ ] **Step 1 : Écrire les tests du service de réservation**

Créez `tests/Service/ReservationServiceTest.php` :
```php
<?php

namespace App\Tests\Service;

use App\Entity\ParkingSpot;
use App\Entity\Reservation;
use App\Entity\User;
use App\Service\ReservationService;
use PHPUnit\Framework\TestCase;

class ReservationServiceTest extends TestCase
{
    public function testCalculatePriceOneHour(): void
    {
        $service = new ReservationService();
        $start = new \DateTimeImmutable('2026-04-15 10:00:00');
        $end = new \DateTimeImmutable('2026-04-15 11:00:00');
        $this->assertEquals(2.50, $service->calculatePrice($start, $end, false));
    }

    public function testCalculatePriceFourHours(): void
    {
        $service = new ReservationService();
        $start = new \DateTimeImmutable('2026-04-15 10:00:00');
        $end = new \DateTimeImmutable('2026-04-15 14:00:00');
        $this->assertEquals(8.00, $service->calculatePrice($start, $end, false));
    }

    public function testCalculatePriceWithSubscriberDiscount(): void
    {
        $service = new ReservationService();
        $start = new \DateTimeImmutable('2026-04-15 10:00:00');
        $end = new \DateTimeImmutable('2026-04-15 11:00:00');
        // Abonné mensuel = -10%
        $this->assertEquals(2.25, $service->calculatePrice($start, $end, true, 'monthly'));
    }
}
```

- [ ] **Step 2 : Lancer les tests pour vérifier qu'ils échouent**

```bash
php bin/phpunit tests/Service/ReservationServiceTest.php
```
Attendu : FAIL.

- [ ] **Step 3 : Créer ReservationService**

Créez `src/Service/ReservationService.php` :
```php
<?php

namespace App\Service;

use App\Entity\ParkingSpot;
use App\Entity\Reservation;
use App\Entity\User;
use App\Repository\ReservationRepository;

class ReservationService
{
    // Tarifs de base en euros
    private const PRICE_PER_HOUR = 2.50;
    private const PRICE_HALF_DAY = 8.00;  // 4h
    private const PRICE_FULL_DAY = 14.00; // 24h

    // Remises abonnés
    private const DISCOUNT_MONTHLY = 0.10; // -10%
    private const DISCOUNT_ANNUAL  = 0.20; // -20%

    public function calculatePrice(
        \DateTimeImmutable $start,
        \DateTimeImmutable $end,
        bool $hasSubscription,
        string $subscriptionPlan = 'monthly'
    ): float {
        $hours = ($end->getTimestamp() - $start->getTimestamp()) / 3600;

        $price = match(true) {
            $hours <= 1   => self::PRICE_PER_HOUR,
            $hours <= 4   => self::PRICE_HALF_DAY,
            default       => self::PRICE_FULL_DAY,
        };

        if ($hasSubscription) {
            $discount = $subscriptionPlan === 'annual' ? self::DISCOUNT_ANNUAL : self::DISCOUNT_MONTHLY;
            $price = round($price * (1 - $discount), 2);
        }

        return $price;
    }

    public function isSpotAvailable(
        ParkingSpot $spot,
        \DateTimeImmutable $start,
        \DateTimeImmutable $end,
        ReservationRepository $reservationRepo
    ): bool {
        $conflicts = $reservationRepo->findConflictingReservations($spot, $start, $end);
        return count($conflicts) === 0;
    }
}
```

- [ ] **Step 4 : Lancer les tests (doivent passer)**

```bash
php bin/phpunit tests/Service/ReservationServiceTest.php
```
Attendu : 3 tests passent.

- [ ] **Step 5 : Ajouter findConflictingReservations dans ReservationRepository**

Ouvrez `src/Repository/ReservationRepository.php` et ajoutez :
```php
public function findConflictingReservations(
    ParkingSpot $spot,
    \DateTimeImmutable $start,
    \DateTimeImmutable $end
): array {
    return $this->createQueryBuilder('r')
        ->where('r.parkingSpot = :spot')
        ->andWhere('r.status IN (:statuses)')
        ->andWhere('r.startTime < :end AND r.endTime > :start')
        ->setParameter('spot', $spot)
        ->setParameter('statuses', ['pending', 'confirmed'])
        ->setParameter('start', $start)
        ->setParameter('end', $end)
        ->getQuery()
        ->getResult();
}
```

- [ ] **Step 6 : Créer ReservationFormType**

Créez `src/Form/ReservationFormType.php` :
```php
<?php

namespace App\Form;

use Symfony\Component\Form\AbstractType;
use Symfony\Component\Form\Extension\Core\Type\DateTimeType;
use Symfony\Component\Form\FormBuilderInterface;
use Symfony\Component\Validator\Constraints\NotBlank;

class ReservationFormType extends AbstractType
{
    public function buildForm(FormBuilderInterface $builder, array $options): void
    {
        $builder
            ->add('startTime', DateTimeType::class, [
                'label' => 'Début de la réservation',
                'widget' => 'single_text',
                'constraints' => [new NotBlank(message: 'Veuillez choisir une date de début')],
            ])
            ->add('endTime', DateTimeType::class, [
                'label' => 'Fin de la réservation',
                'widget' => 'single_text',
                'constraints' => [new NotBlank(message: 'Veuillez choisir une date de fin')],
            ]);
    }
}
```

- [ ] **Step 7 : Créer ReservationController**

Créez `src/Controller/ReservationController.php` :
```php
<?php

namespace App\Controller;

use App\Entity\Reservation;
use App\Form\ReservationFormType;
use App\Repository\ParkingSpotRepository;
use App\Repository\ReservationRepository;
use App\Repository\SubscriptionRepository;
use App\Service\ReservationService;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/reservation')]
class ReservationController extends AbstractController
{
    #[Route('', name: 'app_reservation')]
    public function index(
        Request $request,
        ParkingSpotRepository $spotRepo,
        ReservationRepository $reservationRepo,
        SubscriptionRepository $subscriptionRepo,
        ReservationService $reservationService,
        EntityManagerInterface $em,
    ): Response {
        $spotId = $request->query->getInt('spotId');
        $spot = $spotId ? $spotRepo->find($spotId) : null;

        $form = $this->createForm(ReservationFormType::class);
        $form->handleRequest($request);

        if ($form->isSubmitted() && $form->isValid()) {
            $data = $form->getData();
            $start = \DateTimeImmutable::createFromMutable($data['startTime']);
            $end = \DateTimeImmutable::createFromMutable($data['endTime']);

            if ($end <= $start) {
                $this->addFlash('error', 'La date de fin doit être après la date de début.');
                return $this->redirectToRoute('app_reservation', ['spotId' => $spotId]);
            }

            if (!$spot || !$reservationService->isSpotAvailable($spot, $start, $end, $reservationRepo)) {
                $this->addFlash('error', 'Cette place n\'est pas disponible sur ce créneau.');
                return $this->redirectToRoute('app_reservation');
            }

            // Vérifie si l'utilisateur a un abonnement actif
            $subscription = $subscriptionRepo->findActiveSubscriptionForUser($this->getUser());
            $hasSubscription = $subscription !== null;
            $plan = $subscription?->getPlan() ?? 'monthly';

            $price = $reservationService->calculatePrice($start, $end, $hasSubscription, $plan);

            $reservation = new Reservation();
            $reservation->setUser($this->getUser());
            $reservation->setParkingSpot($spot);
            $reservation->setStartTime($start);
            $reservation->setEndTime($end);
            $reservation->setStatus('pending');
            $reservation->setTotalPrice($price);
            $reservation->setCreatedAt(new \DateTimeImmutable());

            $em->persist($reservation);
            $em->flush();

            return $this->redirectToRoute('app_payment', ['type' => 'reservation', 'id' => $reservation->getId()]);
        }

        return $this->render('reservation/index.html.twig', [
            'spots' => $spotRepo->findBy(['isActive' => true]),
            'selectedSpot' => $spot,
            'form' => $form,
        ]);
    }

    #[Route('/{id}', name: 'app_reservation_show')]
    public function show(Reservation $reservation): Response
    {
        if ($reservation->getUser() !== $this->getUser() && !$this->isGranted('ROLE_ADMIN')) {
            throw $this->createAccessDeniedException();
        }
        return $this->render('reservation/show.html.twig', ['reservation' => $reservation]);
    }

    #[Route('/{id}/cancel', name: 'app_reservation_cancel', methods: ['POST'])]
    public function cancel(Reservation $reservation, EntityManagerInterface $em): Response
    {
        if ($reservation->getUser() !== $this->getUser()) {
            throw $this->createAccessDeniedException();
        }
        $reservation->setStatus('cancelled');
        $em->flush();
        $this->addFlash('success', 'Réservation annulée.');
        return $this->redirectToRoute('app_profile');
    }
}
```

- [ ] **Step 8 : Ajouter findActiveSubscriptionForUser dans SubscriptionRepository**

Ouvrez `src/Repository/SubscriptionRepository.php` et ajoutez :
```php
public function findActiveSubscriptionForUser(User $user): ?Subscription
{
    return $this->createQueryBuilder('s')
        ->where('s.user = :user')
        ->andWhere('s.status = :status')
        ->andWhere('s.endDate > :now')
        ->setParameter('user', $user)
        ->setParameter('status', 'active')
        ->setParameter('now', new \DateTimeImmutable())
        ->setMaxResults(1)
        ->getQuery()
        ->getOneOrNullResult();
}
```

(N'oubliez pas d'ajouter `use App\Entity\User;` et `use App\Entity\Subscription;` en haut du fichier.)

- [ ] **Step 9 : Créer `templates/reservation/index.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Réserver une place{% endblock %}
{% block body %}
<h1>Réserver une place</h1>

<div style="display:grid;grid-template-columns:1fr 1fr;gap:2rem;margin-top:2rem">
    <div>
        <h2>Choisir une place</h2>
        <div class="spots-grid" style="grid-template-columns:repeat(2,1fr)">
            {% for spot in spots %}
            <a href="{{ path('app_reservation', {spotId: spot.id}) }}"
               class="spot-card {{ spot.isIsOccupied ? 'occupied' : 'free' }}"
               style="text-decoration:none">
                <div class="spot-icon">{{ spot.isIsOccupied ? '🚗' : '✅' }}</div>
                <h3>{{ spot.name }}</h3>
                <p class="spot-status">{{ spot.isIsOccupied ? 'Occupée' : 'Libre' }}</p>
            </a>
            {% endfor %}
        </div>
    </div>

    {% if selectedSpot %}
    <div class="card">
        <h2>Réserver {{ selectedSpot.name }}</h2>
        <p style="margin-bottom:1rem;color:#666">
            Tarifs : 1h = 2,50€ | 4h = 8,00€ | 24h = 14,00€
            {% if app.user and app.user.hasActiveSubscription is defined %}
                <br><strong>Réduction abonné appliquée automatiquement</strong>
            {% endif %}
        </p>
        {{ form_start(form) }}
        <div class="form-group">{{ form_label(form.startTime) }}{{ form_widget(form.startTime) }}</div>
        <div class="form-group">{{ form_label(form.endTime) }}{{ form_widget(form.endTime) }}</div>
        <input type="hidden" name="spotId" value="{{ selectedSpot.id }}">
        <button type="submit" class="btn btn-primary" style="width:100%">Continuer vers le paiement</button>
        {{ form_end(form) }}
    </div>
    {% else %}
    <div class="card" style="display:flex;align-items:center;justify-content:center">
        <p style="color:#888">Sélectionnez une place libre pour la réserver.</p>
    </div>
    {% endif %}
</div>
{% endblock %}
```

- [ ] **Step 10 : Créer `templates/reservation/show.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Réservation #{{ reservation.id }}{% endblock %}
{% block body %}
<div class="card">
    <h1>Réservation #{{ reservation.id }}</h1>
    <p><strong>Place :</strong> {{ reservation.parkingSpot.name }}</p>
    <p><strong>Du :</strong> {{ reservation.startTime|date('d/m/Y H:i') }}</p>
    <p><strong>Au :</strong> {{ reservation.endTime|date('d/m/Y H:i') }}</p>
    <p><strong>Montant :</strong> {{ reservation.totalPrice }}€</p>
    <p><strong>Statut :</strong>
        <span class="badge badge-{{ reservation.status }}">{{ reservation.status }}</span>
    </p>

    {% if reservation.status in ['pending', 'confirmed'] %}
    <form method="post" action="{{ path('app_reservation_cancel', {id: reservation.id}) }}" style="margin-top:1rem">
        <input type="hidden" name="_token" value="{{ csrf_token('cancel' ~ reservation.id) }}">
        <button type="submit" class="btn" style="background:#dc3545;color:white"
            onclick="return confirm('Annuler cette réservation ?')">
            Annuler la réservation
        </button>
    </form>
    {% endif %}
</div>
{% endblock %}
```

- [ ] **Step 11 : Commit**

```bash
git add .
git commit -m "feat: système de réservation (formulaire, calcul prix, annulation)"
```

---

## Task 7 : Paiement Stripe et abonnements

**Files:**
- Create: `src/Service/StripeService.php`
- Create: `src/Service/SubscriptionService.php`
- Create: `src/Controller/PaymentController.php`
- Create: `src/Controller/SubscriptionController.php`
- Create: `templates/payment/index.html.twig`
- Create: `templates/subscription/index.html.twig`

- [ ] **Step 1 : Créer StripeService**

Créez `src/Service/StripeService.php` :
```php
<?php

namespace App\Service;

use Stripe\Stripe;
use Stripe\PaymentIntent;
use Stripe\Exception\ApiErrorException;

class StripeService
{
    public function __construct(private string $stripeSecretKey)
    {
        Stripe::setApiKey($this->stripeSecretKey);
    }

    /**
     * Crée un PaymentIntent Stripe pour un montant donné (en euros).
     * Retourne le client_secret à passer au frontend.
     */
    public function createPaymentIntent(float $amount, string $description): array
    {
        $intent = PaymentIntent::create([
            'amount' => (int) ($amount * 100), // Stripe travaille en centimes
            'currency' => 'eur',
            'description' => $description,
            'automatic_payment_methods' => ['enabled' => true],
        ]);

        return [
            'clientSecret' => $intent->client_secret,
            'paymentIntentId' => $intent->id,
        ];
    }

    public function retrievePaymentIntent(string $paymentIntentId): PaymentIntent
    {
        return PaymentIntent::retrieve($paymentIntentId);
    }
}
```

- [ ] **Step 2 : Déclarer StripeService dans `config/services.yaml`**

Ajoutez dans `services:` :
```yaml
    App\Service\StripeService:
        arguments:
            $stripeSecretKey: '%env(STRIPE_SECRET_KEY)%'
```

- [ ] **Step 3 : Créer PaymentController**

Créez `src/Controller/PaymentController.php` :
```php
<?php

namespace App\Controller;

use App\Entity\Payment;
use App\Repository\ReservationRepository;
use App\Repository\SubscriptionRepository;
use App\Service\StripeService;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/paiement')]
class PaymentController extends AbstractController
{
    #[Route('/{type}/{id}', name: 'app_payment')]
    public function index(
        string $type,
        int $id,
        ReservationRepository $reservationRepo,
        SubscriptionRepository $subscriptionRepo,
        StripeService $stripe,
    ): Response {
        $amount = 0;
        $description = '';
        $reservation = null;
        $subscription = null;

        if ($type === 'reservation') {
            $reservation = $reservationRepo->find($id);
            if (!$reservation || $reservation->getUser() !== $this->getUser()) {
                throw $this->createNotFoundException();
            }
            $amount = (float) $reservation->getTotalPrice();
            $description = 'Réservation parking ' . $reservation->getParkingSpot()->getName();
        } elseif ($type === 'subscription') {
            $subscription = $subscriptionRepo->find($id);
            if (!$subscription || $subscription->getUser() !== $this->getUser()) {
                throw $this->createNotFoundException();
            }
            $amount = (float) $subscription->getPrice();
            $description = 'Abonnement ' . $subscription->getPlan();
        }

        $paymentData = $stripe->createPaymentIntent($amount, $description);

        return $this->render('payment/index.html.twig', [
            'clientSecret' => $paymentData['clientSecret'],
            'stripePublicKey' => $_ENV['STRIPE_PUBLIC_KEY'],
            'amount' => $amount,
            'description' => $description,
            'type' => $type,
            'entityId' => $id,
        ]);
    }

    #[Route('/{type}/{id}/success', name: 'app_payment_success')]
    public function success(
        string $type,
        int $id,
        Request $request,
        ReservationRepository $reservationRepo,
        SubscriptionRepository $subscriptionRepo,
        StripeService $stripe,
        EntityManagerInterface $em,
    ): Response {
        $paymentIntentId = $request->query->get('payment_intent');
        $intent = $stripe->retrievePaymentIntent($paymentIntentId);

        if ($intent->status !== 'succeeded') {
            $this->addFlash('error', 'Le paiement a échoué. Veuillez réessayer.');
            return $this->redirectToRoute('app_payment', ['type' => $type, 'id' => $id]);
        }

        $payment = new Payment();
        $payment->setUser($this->getUser());
        $payment->setAmount($intent->amount / 100);
        $payment->setCurrency('EUR');
        $payment->setStatus('succeeded');
        $payment->setStripePaymentIntentId($paymentIntentId);
        $payment->setCreatedAt(new \DateTimeImmutable());

        if ($type === 'reservation') {
            $reservation = $reservationRepo->find($id);
            $reservation->setStatus('confirmed');
            $payment->setReservation($reservation);
        } elseif ($type === 'subscription') {
            $subscription = $subscriptionRepo->find($id);
            $subscription->setStatus('active');
            $payment->setSubscription($subscription);
        }

        $em->persist($payment);
        $em->flush();

        $this->addFlash('success', 'Paiement effectué avec succès !');
        return $this->redirectToRoute('app_profile');
    }
}
```

- [ ] **Step 4 : Créer `templates/payment/index.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Paiement{% endblock %}
{% block body %}
<div class="card">
    <h1>Paiement sécurisé</h1>
    <p style="margin-bottom:1rem"><strong>{{ description }}</strong></p>
    <p style="font-size:1.5rem;color:#e94560;margin-bottom:2rem"><strong>{{ amount }}€</strong></p>

    <div id="payment-element"></div>
    <button id="submit-btn" class="btn btn-primary" style="width:100%;margin-top:1rem">
        Payer {{ amount }}€
    </button>
    <div id="error-message" style="color:red;margin-top:1rem"></div>

    <p style="margin-top:1rem;font-size:0.8rem;color:#888;text-align:center">
        Paiement sécurisé par Stripe — vos données bancaires ne transitent jamais par nos serveurs.
    </p>
</div>
{% endblock %}

{% block javascripts %}
<script src="https://js.stripe.com/v3/"></script>
<script>
const stripe = Stripe('{{ stripePublicKey }}');
const elements = stripe.elements({ clientSecret: '{{ clientSecret }}' });
const paymentElement = elements.create('payment');
paymentElement.mount('#payment-element');

document.getElementById('submit-btn').addEventListener('click', async () => {
    const { error } = await stripe.confirmPayment({
        elements,
        confirmParams: {
            return_url: '{{ url('app_payment_success', {type: type, id: entityId}) }}'
        }
    });
    if (error) {
        document.getElementById('error-message').textContent = error.message;
    }
});
</script>
{% endblock %}
```

- [ ] **Step 5 : Créer SubscriptionService**

Créez `src/Service/SubscriptionService.php` :
```php
<?php

namespace App\Service;

use App\Entity\Subscription;
use App\Entity\User;
use Doctrine\ORM\EntityManagerInterface;

class SubscriptionService
{
    private const PRICES = [
        'monthly' => 30.00,
        'annual'  => 280.00,
    ];

    private const DURATIONS = [
        'monthly' => 30,
        'annual'  => 365,
    ];

    public function createSubscription(User $user, string $plan, EntityManagerInterface $em): Subscription
    {
        $subscription = new Subscription();
        $subscription->setUser($user);
        $subscription->setPlan($plan);
        $subscription->setPrice(self::PRICES[$plan]);
        $subscription->setStartDate(new \DateTimeImmutable());
        $subscription->setEndDate(new \DateTimeImmutable('+' . self::DURATIONS[$plan] . ' days'));
        $subscription->setStatus('pending');

        $em->persist($subscription);
        $em->flush();

        return $subscription;
    }
}
```

- [ ] **Step 6 : Créer SubscriptionController**

Créez `src/Controller/SubscriptionController.php` :
```php
<?php

namespace App\Controller;

use App\Service\SubscriptionService;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/abonnement')]
class SubscriptionController extends AbstractController
{
    #[Route('', name: 'app_subscription')]
    public function index(): Response
    {
        return $this->render('subscription/index.html.twig');
    }

    #[Route('/subscribe/{plan}', name: 'app_subscription_subscribe', methods: ['POST'])]
    public function subscribe(
        string $plan,
        SubscriptionService $subscriptionService,
        EntityManagerInterface $em,
    ): Response {
        if (!in_array($plan, ['monthly', 'annual'])) {
            throw $this->createNotFoundException('Plan invalide.');
        }

        $subscription = $subscriptionService->createSubscription($this->getUser(), $plan, $em);
        return $this->redirectToRoute('app_payment', ['type' => 'subscription', 'id' => $subscription->getId()]);
    }
}
```

- [ ] **Step 7 : Créer `templates/subscription/index.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Abonnements{% endblock %}
{% block body %}
<h1 style="text-align:center;margin-bottom:2rem">Choisissez votre abonnement</h1>

<div style="display:grid;grid-template-columns:1fr 1fr;gap:2rem;max-width:700px;margin:0 auto">
    <div class="card" style="text-align:center;border-top:4px solid #6c757d">
        <h2>Mensuel</h2>
        <p style="font-size:2.5rem;font-weight:bold;color:#e94560;margin:1rem 0">30€<small style="font-size:1rem">/mois</small></p>
        <ul style="list-style:none;margin-bottom:2rem;color:#555">
            <li>✅ Réservation prioritaire</li>
            <li>✅ -10% sur les réservations</li>
            <li>✅ Jusqu'à 7 jours à l'avance</li>
        </ul>
        <form method="post" action="{{ path('app_subscription_subscribe', {plan: 'monthly'}) }}">
            <input type="hidden" name="_token" value="{{ csrf_token('subscribe_monthly') }}">
            <button type="submit" class="btn btn-primary" style="width:100%">Choisir Mensuel</button>
        </form>
    </div>

    <div class="card" style="text-align:center;border-top:4px solid #e94560">
        <div style="background:#e94560;color:white;padding:0.3rem;border-radius:4px;font-size:0.8rem;margin-bottom:1rem">MEILLEURE OFFRE</div>
        <h2>Annuel</h2>
        <p style="font-size:2.5rem;font-weight:bold;color:#e94560;margin:1rem 0">280€<small style="font-size:1rem">/an</small></p>
        <p style="color:#28a745;font-size:0.9rem;margin-bottom:0.5rem">Économisez 80€ par rapport au mensuel</p>
        <ul style="list-style:none;margin-bottom:2rem;color:#555">
            <li>✅ Réservation prioritaire</li>
            <li>✅ -20% sur les réservations</li>
            <li>✅ Jusqu'à 7 jours à l'avance</li>
            <li>✅ Support prioritaire</li>
        </ul>
        <form method="post" action="{{ path('app_subscription_subscribe', {plan: 'annual'}) }}">
            <input type="hidden" name="_token" value="{{ csrf_token('subscribe_annual') }}">
            <button type="submit" class="btn btn-primary" style="width:100%">Choisir Annuel</button>
        </form>
    </div>
</div>
{% endblock %}
```

- [ ] **Step 8 : Commit**

```bash
git add .
git commit -m "feat: paiement Stripe + abonnements mensuel/annuel"
```

---

## Task 8 : Profil utilisateur

**Files:**
- Create: `src/Controller/ProfileController.php`
- Create: `templates/profile/index.html.twig`

- [ ] **Step 1 : Créer ProfileController**

Créez `src/Controller/ProfileController.php` :
```php
<?php

namespace App\Controller;

use App\Repository\ReservationRepository;
use App\Repository\SubscriptionRepository;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/profil')]
#[IsGranted('ROLE_USER')]
class ProfileController extends AbstractController
{
    #[Route('', name: 'app_profile')]
    public function index(
        ReservationRepository $reservationRepo,
        SubscriptionRepository $subscriptionRepo,
    ): Response {
        $user = $this->getUser();
        $reservations = $reservationRepo->findBy(['user' => $user], ['createdAt' => 'DESC']);
        $activeSubscription = $subscriptionRepo->findActiveSubscriptionForUser($user);

        return $this->render('profile/index.html.twig', [
            'reservations' => $reservations,
            'activeSubscription' => $activeSubscription,
        ]);
    }

    #[Route('/supprimer', name: 'app_profile_delete', methods: ['POST'])]
    public function delete(
        Request $request,
        EntityManagerInterface $em,
    ): Response {
        if (!$this->isCsrfTokenValid('delete_account', $request->request->get('_token'))) {
            throw $this->createAccessDeniedException();
        }

        $user = $this->getUser();
        $this->container->get('security.token_storage')->setToken(null);
        $em->remove($user);
        $em->flush();

        $this->addFlash('success', 'Votre compte a été supprimé conformément au RGPD.');
        return $this->redirectToRoute('app_home');
    }
}
```

- [ ] **Step 2 : Créer `templates/profile/index.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Mon profil{% endblock %}
{% block body %}
<h1>Mon profil</h1>

<div style="display:grid;grid-template-columns:1fr 2fr;gap:2rem;margin-top:2rem">
    <div>
        <div class="card" style="margin-bottom:1rem">
            <h2>Mes informations</h2>
            <p><strong>Nom :</strong> {{ app.user.firstName }} {{ app.user.lastName }}</p>
            <p><strong>Email :</strong> {{ app.user.email }}</p>
            <p><strong>Membre depuis :</strong> {{ app.user.createdAt|date('d/m/Y') }}</p>
        </div>

        <div class="card" style="margin-bottom:1rem">
            <h2>Mon abonnement</h2>
            {% if activeSubscription %}
                <span class="badge badge-active">{{ activeSubscription.plan == 'monthly' ? 'Mensuel' : 'Annuel' }}</span>
                <p style="margin-top:0.5rem">Valable jusqu'au {{ activeSubscription.endDate|date('d/m/Y') }}</p>
            {% else %}
                <p style="color:#888">Aucun abonnement actif</p>
                <a href="{{ path('app_subscription') }}" class="btn btn-primary" style="margin-top:1rem">Souscrire</a>
            {% endif %}
        </div>

        <div class="card" style="border:1px solid #dc3545">
            <h2 style="color:#dc3545">Zone danger</h2>
            <p style="margin-bottom:1rem;font-size:0.9rem">La suppression de votre compte est définitive (RGPD).</p>
            <form method="post" action="{{ path('app_profile_delete') }}"
                  onsubmit="return confirm('Supprimer définitivement votre compte ?')">
                <input type="hidden" name="_token" value="{{ csrf_token('delete_account') }}">
                <button type="submit" class="btn" style="background:#dc3545;color:white;width:100%">
                    Supprimer mon compte
                </button>
            </form>
        </div>
    </div>

    <div>
        <h2>Mes réservations</h2>
        {% if reservations is empty %}
            <p style="color:#888;margin-top:1rem">Aucune réservation pour l'instant.
                <a href="{{ path('app_reservation') }}">Réserver une place →</a>
            </p>
        {% else %}
            <table style="margin-top:1rem">
                <tr><th>Place</th><th>Du</th><th>Au</th><th>Prix</th><th>Statut</th><th>Action</th></tr>
                {% for r in reservations %}
                <tr>
                    <td>{{ r.parkingSpot.name }}</td>
                    <td>{{ r.startTime|date('d/m H:i') }}</td>
                    <td>{{ r.endTime|date('d/m H:i') }}</td>
                    <td>{{ r.totalPrice }}€</td>
                    <td><span class="badge badge-{{ r.status }}">{{ r.status }}</span></td>
                    <td><a href="{{ path('app_reservation_show', {id: r.id}) }}">Voir</a></td>
                </tr>
                {% endfor %}
            </table>
        {% endif %}
    </div>
</div>
{% endblock %}
```

- [ ] **Step 3 : Commit**

```bash
git add .
git commit -m "feat: profil utilisateur (historique réservations, abonnement, suppression RGPD)"
```

---

## Task 9 : Panneau d'administration

**Files:**
- Create: `src/Controller/Admin/AdminController.php`
- Create: `templates/admin/index.html.twig`
- Create: `templates/admin/spots.html.twig`
- Create: `templates/admin/reservations.html.twig`
- Create: `templates/admin/users.html.twig`

- [ ] **Step 1 : Créer AdminController**

Créez `src/Controller/Admin/AdminController.php` :
```php
<?php

namespace App\Controller\Admin;

use App\Repository\ParkingSpotRepository;
use App\Repository\ReservationRepository;
use App\Repository\SubscriptionRepository;
use App\Repository\UserRepository;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Attribute\IsGranted;

#[Route('/admin')]
#[IsGranted('ROLE_ADMIN')]
class AdminController extends AbstractController
{
    #[Route('', name: 'app_admin')]
    public function index(
        ParkingSpotRepository $spotRepo,
        ReservationRepository $reservationRepo,
        SubscriptionRepository $subscriptionRepo,
        UserRepository $userRepo,
    ): Response {
        $totalRevenue = $reservationRepo->createQueryBuilder('r')
            ->select('SUM(r.totalPrice)')
            ->where('r.status = :status')
            ->setParameter('status', 'confirmed')
            ->getQuery()->getSingleScalarResult() ?? 0;

        return $this->render('admin/index.html.twig', [
            'totalSpots'       => count($spotRepo->findAll()),
            'freeSpots'        => $spotRepo->count(['isOccupied' => false, 'isActive' => true]),
            'totalUsers'       => $userRepo->count([]),
            'totalReservations' => $reservationRepo->count([]),
            'activeSubscriptions' => $subscriptionRepo->count(['status' => 'active']),
            'totalRevenue'     => $totalRevenue,
        ]);
    }

    #[Route('/places', name: 'app_admin_spots')]
    public function spots(ParkingSpotRepository $spotRepo): Response
    {
        return $this->render('admin/spots.html.twig', ['spots' => $spotRepo->findAll()]);
    }

    #[Route('/places/{id}/toggle', name: 'app_admin_spot_toggle', methods: ['POST'])]
    public function toggleSpot(int $id, ParkingSpotRepository $spotRepo, EntityManagerInterface $em): Response
    {
        $spot = $spotRepo->find($id);
        $spot->setIsActive(!$spot->isIsActive());
        $em->flush();
        $this->addFlash('success', 'Place mise à jour.');
        return $this->redirectToRoute('app_admin_spots');
    }

    #[Route('/reservations', name: 'app_admin_reservations')]
    public function reservations(ReservationRepository $reservationRepo): Response
    {
        return $this->render('admin/reservations.html.twig', [
            'reservations' => $reservationRepo->findBy([], ['createdAt' => 'DESC']),
        ]);
    }

    #[Route('/utilisateurs', name: 'app_admin_users')]
    public function users(UserRepository $userRepo): Response
    {
        return $this->render('admin/users.html.twig', ['users' => $userRepo->findAll()]);
    }
}
```

- [ ] **Step 2 : Créer `templates/admin/index.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Administration{% endblock %}
{% block body %}
<h1>Tableau de bord administrateur</h1>

<div style="display:grid;grid-template-columns:repeat(3,1fr);gap:1.5rem;margin-top:2rem">
    <div class="card" style="text-align:center">
        <p style="color:#888;font-size:0.9rem">Places libres</p>
        <p style="font-size:2.5rem;font-weight:bold;color:#28a745">{{ freeSpots }}/{{ totalSpots }}</p>
    </div>
    <div class="card" style="text-align:center">
        <p style="color:#888;font-size:0.9rem">Utilisateurs</p>
        <p style="font-size:2.5rem;font-weight:bold">{{ totalUsers }}</p>
    </div>
    <div class="card" style="text-align:center">
        <p style="color:#888;font-size:0.9rem">Réservations totales</p>
        <p style="font-size:2.5rem;font-weight:bold">{{ totalReservations }}</p>
    </div>
    <div class="card" style="text-align:center">
        <p style="color:#888;font-size:0.9rem">Abonnements actifs</p>
        <p style="font-size:2.5rem;font-weight:bold;color:#e94560">{{ activeSubscriptions }}</p>
    </div>
    <div class="card" style="text-align:center;grid-column:span 2">
        <p style="color:#888;font-size:0.9rem">Revenus confirmés</p>
        <p style="font-size:2.5rem;font-weight:bold;color:#1a1a2e">{{ totalRevenue|number_format(2, ',', ' ') }}€</p>
    </div>
</div>

<div style="display:flex;gap:1rem;margin-top:2rem">
    <a href="{{ path('app_admin_spots') }}" class="btn btn-primary">Gérer les places</a>
    <a href="{{ path('app_admin_reservations') }}" class="btn btn-secondary" style="background:#1a1a2e;color:white">Réservations</a>
    <a href="{{ path('app_admin_users') }}" class="btn btn-secondary" style="background:#6c757d;color:white">Utilisateurs</a>
</div>
{% endblock %}
```

- [ ] **Step 3 : Créer `templates/admin/spots.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Admin — Places{% endblock %}
{% block body %}
<h1>Gestion des places</h1>
<a href="{{ path('app_admin') }}" style="display:inline-block;margin-bottom:1rem">← Retour</a>
<table style="margin-top:1rem">
    <tr><th>ID</th><th>Nom</th><th>État</th><th>Dernière mise à jour</th><th>Active</th><th>Action</th></tr>
    {% for spot in spots %}
    <tr>
        <td>{{ spot.id }}</td>
        <td>{{ spot.name }}</td>
        <td><span class="badge {{ spot.isIsOccupied ? 'badge-occupied' : 'badge-free' }}">
            {{ spot.isIsOccupied ? 'Occupée' : 'Libre' }}
        </span></td>
        <td>{{ spot.lastUpdatedAt ? spot.lastUpdatedAt|date('d/m/Y H:i:s') : 'Jamais' }}</td>
        <td>{{ spot.isIsActive ? '✅' : '❌' }}</td>
        <td>
            <form method="post" action="{{ path('app_admin_spot_toggle', {id: spot.id}) }}" style="display:inline">
                <input type="hidden" name="_token" value="{{ csrf_token('toggle' ~ spot.id) }}">
                <button type="submit" class="btn btn-sm" style="background:{{ spot.isIsActive ? '#dc3545' : '#28a745' }};color:white">
                    {{ spot.isIsActive ? 'Désactiver' : 'Activer' }}
                </button>
            </form>
        </td>
    </tr>
    {% endfor %}
</table>
{% endblock %}
```

- [ ] **Step 4 : Créer `templates/admin/reservations.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Admin — Réservations{% endblock %}
{% block body %}
<h1>Toutes les réservations</h1>
<a href="{{ path('app_admin') }}" style="display:inline-block;margin-bottom:1rem">← Retour</a>
<table>
    <tr><th>ID</th><th>Utilisateur</th><th>Place</th><th>Du</th><th>Au</th><th>Prix</th><th>Statut</th></tr>
    {% for r in reservations %}
    <tr>
        <td>#{{ r.id }}</td>
        <td>{{ r.user.firstName }} {{ r.user.lastName }}</td>
        <td>{{ r.parkingSpot.name }}</td>
        <td>{{ r.startTime|date('d/m H:i') }}</td>
        <td>{{ r.endTime|date('d/m H:i') }}</td>
        <td>{{ r.totalPrice }}€</td>
        <td><span class="badge badge-{{ r.status }}">{{ r.status }}</span></td>
    </tr>
    {% endfor %}
</table>
{% endblock %}
```

- [ ] **Step 5 : Créer `templates/admin/users.html.twig`**

```twig
{% extends 'base.html.twig' %}
{% block title %}Admin — Utilisateurs{% endblock %}
{% block body %}
<h1>Utilisateurs inscrits</h1>
<a href="{{ path('app_admin') }}" style="display:inline-block;margin-bottom:1rem">← Retour</a>
<table>
    <tr><th>ID</th><th>Nom</th><th>Email</th><th>Rôle</th><th>Inscrit le</th></tr>
    {% for user in users %}
    <tr>
        <td>{{ user.id }}</td>
        <td>{{ user.firstName }} {{ user.lastName }}</td>
        <td>{{ user.email }}</td>
        <td>{{ 'ROLE_ADMIN' in user.roles ? '👑 Admin' : 'Utilisateur' }}</td>
        <td>{{ user.createdAt|date('d/m/Y') }}</td>
    </tr>
    {% endfor %}
</table>
{% endblock %}
```

- [ ] **Step 6 : Commit**

```bash
git add .
git commit -m "feat: panneau administration (stats, places, réservations, utilisateurs)"
```

---

## Task 10 : En-têtes de sécurité HTTP

**Files:**
- Create: `src/EventSubscriber/SecurityHeadersSubscriber.php`

- [ ] **Step 1 : Créer SecurityHeadersSubscriber**

Créez `src/EventSubscriber/SecurityHeadersSubscriber.php` :
```php
<?php

namespace App\EventSubscriber;

use Symfony\Component\EventDispatcher\EventSubscriberInterface;
use Symfony\Component\HttpKernel\Event\ResponseEvent;
use Symfony\Component\HttpKernel\KernelEvents;

class SecurityHeadersSubscriber implements EventSubscriberInterface
{
    public static function getSubscribedEvents(): array
    {
        return [KernelEvents::RESPONSE => 'onKernelResponse'];
    }

    public function onKernelResponse(ResponseEvent $event): void
    {
        if (!$event->isMainRequest()) return;

        $response = $event->getResponse();
        $response->headers->set('X-Frame-Options', 'DENY');
        $response->headers->set('X-Content-Type-Options', 'nosniff');
        $response->headers->set('Referrer-Policy', 'strict-origin-when-cross-origin');
        $response->headers->set('Permissions-Policy', 'camera=(), microphone=(), geolocation=()');
    }
}
```

- [ ] **Step 2 : Vérifier que le subscriber est chargé**

```bash
php bin/console debug:event-dispatcher kernel.response
```
Vous devez voir `App\EventSubscriber\SecurityHeadersSubscriber` dans la liste.

- [ ] **Step 3 : Commit final**

```bash
git add .
git commit -m "feat: en-têtes sécurité HTTP (X-Frame-Options, nosniff, Referrer-Policy)"
```

---

## Task 11 : Code Arduino ESP32

Le code embarqué se trouve dans le dossier `esp32/` à la racine du repo. Cette tâche se contente de vérifier que les fichiers existent et de configurer/flasher chaque nœud.

**Architecture matérielle** (voir `esp32/README.md` pour le câblage GPIO complet) :

| Nœud | Sketch | Rôle | Capteurs | LEDs | OLEDs |
|------|--------|------|----------|------|-------|
| #1 "panneau" | `esp32/node-1.ino` | Capteurs places 1-2 + panneau d'information à l'entrée | 2× HC-SR04A | 2 | **2× SSD1306 SPI** |
| #2 | `esp32/node-sensor.ino` | Capteurs places 3-4 | 2× HC-SR04A | 2 | — |
| #3 | `esp32/node-sensor.ino` | Capteurs places 5-6 | 2× HC-SR04A | 2 | — |

**Files:**
- Existing: `esp32/node-1.ino`
- Existing: `esp32/node-sensor.ino`
- Existing: `esp32/README.md`

**Bibliothèques Arduino requises :**
- `WiFi`, `HTTPClient` (intégrées au core ESP32)
- `ArduinoJson` (Benoit Blanchon)
- `Adafruit GFX Library` + `Adafruit SSD1306` (uniquement pour le nœud #1)

- [ ] **Step 1 : Vérifier la présence des fichiers**

```bash
ls esp32/
# attendu : node-1.ino  node-sensor.ino  README.md
```

- [ ] **Step 2 : Configurer chaque sketch**

Dans chaque `.ino`, remplacer en haut du fichier :
```cpp
const char* WIFI_SSID     = "...";        // votre SSID
const char* WIFI_PASSWORD = "...";        // votre mot de passe
const char* SERVER_URL    = "https://...";// URL du serveur Symfony, sans slash final
const char* API_KEY       = "...";        // doit matcher la clé X-API-Key côté Symfony
```

Pour `node-sensor.ino`, **commenter/décommenter le bon bloc** pour choisir entre node-2 (places 3-4) et node-3 (places 5-6).

- [ ] **Step 3 : Flasher les 3 nœuds**

Avec l'Arduino IDE (Board: "ESP32 Dev Module") :
- ESP32 #1 → flash `node-1.ino`
- ESP32 #2 → flash `node-sensor.ino` avec `NODE_ID = "node-2"`
- ESP32 #3 → flash `node-sensor.ino` avec `NODE_ID = "node-3"`

- [ ] **Step 4 : Vérification au moniteur série (115200 baud)**

Sur chaque nœud, on doit voir :
```
[node-X] Boot ...
[WiFi] OK 192.168.x.x
[POST] place N LIBRE (47.3 cm) HTTP 200
```

Sur le nœud #1, le panneau OLED affiche aussi en plus :
- Gauche : grille des 6 places (LIB / OCC)
- Droite : "X/6 LIBRES" + heure

- [ ] **Step 5 : Commit**

```bash
git add esp32/
git commit -m "feat(esp32): code embarqué — node-1 panneau + node-sensor"
```

---

## Vérification finale

- [ ] `symfony server:start -d` → site accessible sur `http://127.0.0.1:8000`
- [ ] Inscription et connexion fonctionnent
- [ ] Dashboard affiche 6 places et se rafraîchit
- [ ] `curl POST /api/spot/1` avec la clé API change l'état de la place
- [ ] Sans clé API → réponse 401
- [ ] Réservation crée bien une entrée en BDD
- [ ] Page abonnement affiche les 2 plans
- [ ] Page admin accessible avec `admin@parking.fr` / `admin1234`
- [ ] Page admin inaccessible avec le compte utilisateur normal
- [ ] `php bin/phpunit` → tous les tests passent

---

## Comptes de test (créés par les fixtures)

| Rôle | Email | Mot de passe |
|------|-------|-------------|
| Admin | admin@parking.fr | admin1234 |
| Utilisateur | user@parking.fr | user1234 |

---

## Note Stripe

Pour tester le paiement sans vrai compte bancaire, utilisez la carte de test Stripe :
- Numéro : `4242 4242 4242 4242`
- Date : n'importe quelle date future
- CVC : n'importe quel 3 chiffres
