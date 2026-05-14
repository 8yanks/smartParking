# Design — Site Web Parking Intelligent

**Projet :** Parking Intelligent — Équipe 154  
**Membres :** Michaux Yanis, Perret Théo, Bruneau Adam, Malespine James  
**Date :** 2026-04-14  
**Contexte :** Projet scolaire — semestre universitaire (~16-18 semaines)

---

## 1. Vue d'ensemble

Site web en **Symfony 7 (PHP 8.3)** pour gérer un parking intelligent de **6 places** équipées de capteurs ultrasons HC-SR04A pilotés par des **ESP32 NodeMCU**.

Côté matériel (voir [`esp32/README.md`](../../../esp32/README.md) pour le câblage détaillé) :
- **3 ESP32 NodeMCU** (2 capteurs chacun)
- **6 capteurs HC-SR04A** (un par place)
- **6 LEDs rouges** (1 par place, allumée si occupée — feedback visuel local)
- **2 OLEDs SSD1306 I2C (double-bus)** montés ensemble sur le nœud #1, formant un **panneau d'information à l'entrée** (vue carte des 6 places + compteur "X/6 libres" + heure)

Le site expose :
- Une **API REST sécurisée** pour recevoir les données des ESP32 en temps réel
- Un **dashboard public** affichant l'état des 6 places
- Un **système de réservation** avec paiement et abonnement
- Un **panneau d'administration** pour l'exploitant du parking

---

## 2. Stack technique

| Composant | Technologie |
|-----------|-------------|
| Framework | Symfony 7 (PHP 8.3) |
| Base de données | MySQL 8 |
| Templates | Twig |
| ORM | Doctrine |
| Authentification | Symfony Security Component |
| Paiement | Stripe SDK PHP |
| Frontend | HTML/CSS + JavaScript vanilla (fetch API) |
| Serveur local | Symfony CLI / XAMPP |
| Serveur prod | Apache/Nginx avec HTTPS (Let's Encrypt) |

---

## 2 bis. Matériel embarqué

| Composant | Quantité | Rôle |
|-----------|----------|------|
| ESP32 NodeMCU | 3 | Microcontrôleur WiFi pilotant les capteurs/LEDs/écrans |
| Capteur ultrason HC-SR04A | 6 | Mesure la présence d'un véhicule (1 par place) |
| LED rouge 5 mm + R 220 Ω | 6 (+1 spare) | Indicateur visuel local par place (allumée si occupée) |
| OLED SBC-OLED01-V2 (SSD1306 128×64, I2C double-bus) | 2 | Panneau d'information à l'entrée — montés sur le nœud #1 |
| Câbles Dupont, breadboard, alim USB 5V | — | Câblage et alim |

**Répartition par nœud** :
- **Nœud #1 "panneau"** : capteurs places 1-2 + 2 LEDs + **2 OLEDs I2C** formant le panneau
- **Nœud #2** : capteurs places 3-4 + 2 LEDs
- **Nœud #3** : capteurs places 5-6 + 2 LEDs

Code embarqué : voir `esp32/node-1.ino` (panneau) et `esp32/node-sensor.ino` (réutilisé pour #2 et #3). Câblage GPIO détaillé : voir [`esp32/README.md`](../../../esp32/README.md).

---

## 3. Architecture

```
                              ┌──────── ESP32 #1 "panneau" ────────┐
                              │  2 capteurs (places 1-2)           │
                              │  2 LEDs locales                    │
                              │  2 OLED SSD1306 I2C                │
                              │  ▲ POST /api/spot/{1,2}            │
                              │  ▼ GET  /api/spots (panneau)       │
                              └──────────────┬─────────────────────┘
                                             │
[ESP32 #2 capteurs places 3-4] ──────────────┤  HTTPS + X-API-Key
[ESP32 #3 capteurs places 5-6] ──────────────┤
                                             ▼
                                  [API REST Symfony]  ──►  [MySQL Database]
                                             │
                              [Symfony Controllers / Services]
                                             │
                    ┌────────────────────────┼────────────────────────┐
               [Dashboard]            [Réservation]            [Admin Panel]
               [Temps réel]           [Paiement]               [Statistiques]
                                      [Abonnement]
```

**Communication ESP32 → Serveur :**
- Chaque ESP32 envoie un `POST /api/spot/{id}` toutes les ~5 secondes
- Header `X-API-Key: <clé_secrète>` obligatoire
- Body JSON : `{"occupied": true, "esp32_id": "node-1", "distance_cm": 12.5}`
- Réponse serveur : `200 OK` ou `401 Unauthorized`

**Communication Serveur → ESP32 #1 (panneau) :**
- Le nœud #1 appelle `GET /api/spots` toutes les ~5 secondes pour rafraîchir l'affichage du panneau OLED
- Réponse : `[{"id":1, "name":"Place A1", "is_occupied":false}, ...]`

**Rafraîchissement dashboard :**
- Le navigateur appelle `GET /api/spots` toutes les 5 secondes via `fetch()`
- Mise à jour dynamique du DOM sans rechargement de page

**Feedback physique pour l'usager :**
- LED rouge allumée à côté de chaque place occupée (réaction immédiate, pilotée localement par l'ESP32 du nœud)
- Panneau OLED double à l'entrée du parking : carte des 6 places + nombre de places libres en gros chiffres + heure

---

## 4. Base de données — Entités Doctrine

### `User`
```
id, email, password (hashed), first_name, last_name,
role (ROLE_USER | ROLE_ADMIN), created_at, rgpd_consent_at
```

### `ParkingSpot`
```
id (1-6), name (ex: "Place A1"), is_occupied (bool),
last_updated_at, is_active (bool)
```

### `SensorData`
```
id, parking_spot_id (FK), is_occupied (bool),
distance_cm (float), esp32_id (string), recorded_at
```

### `Reservation`
```
id, user_id (FK), parking_spot_id (FK),
start_time, end_time, status (pending|confirmed|cancelled|completed),
total_price, created_at
```

### `Subscription`
```
id, user_id (FK), plan (monthly|annual),
price (30.00|280.00), start_date, end_date,
status (active|expired|cancelled), stripe_subscription_id
```

### `Payment`
```
id, user_id (FK), reservation_id (FK, nullable),
subscription_id (FK, nullable), amount, currency (EUR),
status (pending|succeeded|failed), stripe_payment_intent_id, created_at
```

---

## 5. Pages et routes

### Publiques
| Route | Description |
|-------|-------------|
| `GET /` | Page d'accueil — présentation du parking, état général |
| `GET /register` | Formulaire d'inscription |
| `POST /register` | Traitement inscription |
| `GET /login` | Formulaire de connexion |
| `POST /login` | Traitement connexion (géré par Symfony Security) |
| `GET /logout` | Déconnexion |
| `GET /dashboard` | Vue temps réel des 6 places (accessible sans compte) |

### Utilisateur authentifié (`ROLE_USER`)
| Route | Description |
|-------|-------------|
| `GET /reservation` | Liste des places disponibles + formulaire de réservation |
| `POST /reservation` | Créer une réservation |
| `GET /reservation/{id}` | Détail d'une réservation |
| `POST /reservation/{id}/cancel` | Annuler une réservation |
| `GET /abonnement` | Page choix d'abonnement (Mensuel / Annuel) |
| `POST /abonnement` | Souscrire à un abonnement |
| `GET /paiement/{type}` | Page de paiement Stripe |
| `GET /profil` | Profil utilisateur, historique, abonnement actif |
| `POST /profil/supprimer` | Suppression du compte (RGPD) |

### Administrateur (`ROLE_ADMIN`)
| Route | Description |
|-------|-------------|
| `GET /admin` | Dashboard stats (taux d'occupation, revenus) |
| `GET /admin/places` | Gestion des 6 places |
| `POST /admin/places/{id}/toggle` | Activer/désactiver une place |
| `GET /admin/reservations` | Liste toutes les réservations |
| `GET /admin/utilisateurs` | Liste tous les utilisateurs |
| `GET /admin/abonnements` | Liste tous les abonnements |

### API REST (ESP32)
| Route | Description |
|-------|-------------|
| `POST /api/spot/{id}` | Mise à jour état d'une place (auth: X-API-Key) |
| `GET /api/spots` | État de toutes les places (public, pour le dashboard JS) |

---

## 6. Sécurité

| Mesure | Implémentation |
|--------|---------------|
| HTTPS / TLS 1.3 | Certificat Let's Encrypt en production, auto-signé en dev |
| Auth ESP32 | Header `X-API-Key` vérifié dans un EventSubscriber Symfony |
| Mots de passe | Hachage `bcrypt` via `PasswordHasher` Symfony |
| CSRF | Tokens CSRF Symfony sur tous les formulaires |
| Rôles | `ROLE_USER` et `ROLE_ADMIN` — firewall Symfony |
| Validation | Symfony Validator sur toutes les entités et formulaires |
| Rate limiting | `symfony/rate-limiter` sur les endpoints API (60 req/min) |
| RGPD | Collecte minimale, consentement enregistré, suppression compte |
| Variables sensibles | Clés API et Stripe dans `.env.local` (jamais commitées) |
| Headers sécurité | `X-Frame-Options`, `X-Content-Type-Options`, `Content-Security-Policy` |

---

## 7. Plans d'abonnement

| Plan | Prix | Durée | Avantages |
|------|------|-------|-----------|
| Mensuel | 30 €/mois | 30 jours | Réservation prioritaire, -10% sur les réservations à l'heure |
| Annuel | 280 €/an | 365 jours | Économie de 80€/an, réservation prioritaire, -20% sur les réservations |

Les abonnés peuvent réserver jusqu'à 7 jours à l'avance (vs 24h pour les non-abonnés).

---

## 8. Tarification des réservations

| Durée | Prix |
|-------|------|
| 1 heure | 2,50 € |
| Demi-journée (4h) | 8,00 € |
| Journée (24h) | 14,00 € |

Remises appliquées automatiquement pour les abonnés actifs.

---

## 9. Structure des dossiers Symfony

```
siteReservation/
├── config/
│   ├── packages/
│   └── routes/
├── src/
│   ├── Controller/
│   │   ├── HomeController.php
│   │   ├── DashboardController.php
│   │   ├── ReservationController.php
│   │   ├── SubscriptionController.php
│   │   ├── PaymentController.php
│   │   ├── ProfileController.php
│   │   ├── Admin/
│   │   │   └── AdminController.php
│   │   └── Api/
│   │       └── SpotApiController.php
│   ├── Entity/
│   │   ├── User.php
│   │   ├── ParkingSpot.php
│   │   ├── SensorData.php
│   │   ├── Reservation.php
│   │   ├── Subscription.php
│   │   └── Payment.php
│   ├── Repository/
│   ├── Form/
│   ├── Service/
│   │   ├── ParkingService.php
│   │   ├── ReservationService.php
│   │   ├── SubscriptionService.php
│   │   └── StripeService.php
│   └── Security/
│       └── ApiKeyAuthenticator.php
├── templates/
│   ├── base.html.twig
│   ├── home/
│   ├── dashboard/
│   ├── reservation/
│   ├── subscription/
│   ├── profile/
│   └── admin/
├── public/
│   ├── css/
│   └── js/
│       └── dashboard.js   ← polling toutes les 5s
├── migrations/
├── .env
└── .env.local             ← secrets (jamais commité)
```

---

## 10. Flux utilisateur type

**Scénario : un utilisateur réserve une place**

1. L'utilisateur se connecte sur `/login`
2. Il consulte `/dashboard` — voit la place 3 libre
3. Il va sur `/reservation`, choisit la place 3, créneaux 14h-16h
4. Il est redirigé vers `/paiement/reservation` — paie via Stripe
5. La réservation passe en statut `confirmed`
6. À 14h, l'utilisateur arrive sur place — il voit sur le panneau OLED à l'entrée que la place 3 est encore libre
7. L'ESP32 du nœud capteur détecte sa présence → la place passe à `occupied`, la LED rouge locale s'allume, et le panneau OLED se met à jour au prochain rafraîchissement

**Scénario : l'ESP32 envoie des données**

1. ESP32 mesure la distance avec le capteur ultrason
2. Si distance < 20 cm → place occupée
3. ESP32 envoie `POST /api/spot/3` avec `{"occupied": true, "distance_cm": 8.2}`
4. Symfony vérifie la clé API, met à jour `parking_spot` et insère dans `sensor_data`
5. Le dashboard se rafraîchit automatiquement en 5 secondes max
