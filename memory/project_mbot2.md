---
name: project-mbot2
description: Le projet smartParking inclut maintenant des robots mBot2 pour suivre un circuit noir au sol
metadata:
  type: project
---

Deux robots mBot2 (CyberPi + Quad RGB Sensor) doivent suivre un circuit formé d'une bande/carré noir au sol, avec des obstacles à déplacer.

**Why:** Extension physique du projet scolaire "Parking Intelligent" équipe 112 — les robots simulent probablement des véhicules sur le circuit.

**How to apply:** Le code robot est dans `mbot2/` (séparé du site Next.js et des ESP32). L'environnement est mBlock 5 mode Python. Commencer par un seul robot, puis étendre au deuxième. L'obstacle est la prochaine étape après le suivi de ligne.
