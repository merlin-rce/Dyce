#include "rng.h"
#include "esp_random.h"

// ============================================================
// DYCE randomness - 100% offline.
//
//   esp_random() = HARDWARE RANDOM generator of the ESP32-S3 (internal
//   electrical noise). It's real randomness, no network, no dependency.
//
//   rng_roll(n) does "rejection sampling" : we throw away the rare high
//   draws that don't land right, so each value 1..n is EXACTLY
//   equally likely (a plain "% n" would be slightly biased).
// ============================================================

int rng_roll(int n) {
    if (n < 2) return n;
    uint32_t rejet = (uint32_t)(0x100000000ULL % (uint32_t)n);   // 2^32 mod n
    uint32_t r;
    do { r = esp_random(); } while (r < rejet);
    return (int)(r % n) + 1;
}
