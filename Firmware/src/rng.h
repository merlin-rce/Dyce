#pragma once

// Hasard MATERIEL de l'ESP32 (esp_random) + rejection sampling.
// 100% hors-ligne, rien a configurer.
int  rng_roll(int n);   // entier uniforme dans [1, n]
