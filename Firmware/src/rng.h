#pragma once

// HARDWARE randomness from the ESP32 (esp_random) + rejection sampling.
// 100% offline, nothing to set up.
int  rng_roll(int n);   // uniform integer in [1, n]
