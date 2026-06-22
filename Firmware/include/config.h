#pragma once

// ===== DYCE settings =====
//Screen rotation (0, 1, 2, 3) : 0 = portrait, 1 = landscape, 2 = portrait flipped, 3 = landscape flipped
#define SCREEN_ROTATION 0

// Encoder pins
#define PIN_ENC_A 6
#define PIN_ENC_B 5
#define ENC_DIR   1        // turn direction (-1 to flip)

// "1 in N" categories (counter-clockwise to change). After the last value
// comes the "Contact" category (index = N_COUNT) that shows a QR code.
static const int N_VALUES[] = { 2, 5, 10, 50, 99 };
static const int N_COUNT    = 5;
#define N_START 1          // starts on the 5

// Ring fill : big steps at the bottom, small at the top -> few notches
#define STEP_MAX_INC 28
#define STEP_MIN_INC 10
#define FILL_SPEED   8     // shown fill speed
#define FRAME_MS     33    // refresh (~30 fps)
#define COOLDOWN_MS  250   // small delay after a roll (spam ok, but not too much)

// Reveal : anim then the number stays a bit, then it clears
#define REVEAL_MS  2500

// Secret : stop around 2/3 -> lands on 67
#define SECRET_LO 55
#define SECRET_HI 80
#define SECRET_MS 1500

// Boot intro + idle mode
#define INTRO_MS   4500    // intro movie, paced
#define ATTRACT_MS 10000   // without touching anything -> clears the number, back to the tips
#define FADE_MS    900     // soft fade-in of the tips
#define ATTRACT_OUT_MS 250 // fast fade-out of the tips when you start turning
#define FIRST_MS   8000    // the very 1st tip stays longer
#define HEHE_MS    6000    // the secret 67 stays shown this long then disappears
