#include "rng.h"
#include "esp_random.h"

// ============================================================
// Hasard de DYCE - 100% hors-ligne.
//
//   esp_random() = generateur ALEATOIRE MATERIEL de l'ESP32-S3 (bruit
//   electrique interne). C'est du vrai hasard, sans reseau ni dependance.
//
//   rng_roll(n) fait du "rejection sampling" : on jette les rares tirages du
//   haut qui ne tombent pas juste, pour que chaque valeur 1..n soit EXACTEMENT
//   equiprobable (un simple "% n" serait legerement biaise).
// ============================================================

int rng_roll(int n) {
    if (n < 2) return n;
    uint32_t rejet = (uint32_t)(0x100000000ULL % (uint32_t)n);   // 2^32 mod n
    uint32_t r;
    do { r = esp_random(); } while (r < rejet);
    return (int)(r % n) + 1;
}
