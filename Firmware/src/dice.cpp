#include "dice.h"
#include "config.h"
#include "rng.h"
#include <Arduino.h>

// The game goes through 4 phases :
//   INTRO    : the startup animation
//   ATTRACT  : idle (no number), we show tips
//   CHARGE   : you turn to fill the ring
//   REVEAL   : the ring is full -> we show the rolled number
enum { INTRO, ATTRACT, CHARGE, REVEAL };

static int      nombre   = 7;
static int      indexN   = N_START;   // 0..N_COUNT ; N_COUNT = Contact category
static int      charge   = 0;         // target fill 0..100
static int      affiche  = 0;         // shown fill (smoothly follows "charge")
static int      flash    = 0;
static int      phase    = INTRO;
static bool     special  = false;     // the secret 67
static bool     montre   = false;     // is there a number to show ?
static uint32_t introStart   = 0;
static uint32_t revDebut     = 0;
static int      revMs        = 0;     // duration of the current REVEAL phase
static uint32_t tRoll        = 0;     // time of the last roll (for the cooldown)
static uint32_t zoneDebut    = 0;     // since when we're in the 2/3 zone (0 = outside)
static uint32_t lastInput    = 0;     // last action on the encoder
static uint32_t attractStart = 0;     // time when we went idle
static uint32_t attractLeft  = 0;     // time when we left idle
static uint32_t specialStart = 0;     // time when the secret 67 appeared

static bool enContact() { return indexN == N_COUNT; }   // last category = QR

// Step size : big when the ring is low, small when it's high.
static int pas() {
    int p = STEP_MIN_INC + (STEP_MAX_INC - STEP_MIN_INC) * (100 - charge) / 100;
    p += (int)rng_roll(3) - 2;          // a bit of irregularity
    return p < 1 ? 1 : p;
}

// Back to idle (clears the number, starts the tips fade).
static void versAttente() {
    phase = ATTRACT; attractStart = millis();
    special = false; montre = false; charge = 0; affiche = 0;
}

// Normal roll when the ring is full.
static void lanceReveal() {
    nombre  = rng_roll(N_VALUES[indexN]);
    special = false; montre = true; revMs = REVEAL_MS;
    phase = REVEAL; revDebut = millis(); tRoll = millis();
    flash = 100; zoneDebut = 0;
}

// Hidden menu : you open it by stopping around 2/3 (no need to finish the ring).
// It shows 67 for 6 s then goes back to idle on its own.
static void lanceSecret() {
    nombre = 67; special = true; montre = true; specialStart = millis();
    revMs = HEHE_MS;
    phase = REVEAL; revDebut = millis(); tRoll = millis();
    flash = 100; zoneDebut = 0;
}

void dice_begin() {
    nombre = rng_roll(N_VALUES[indexN]);
    phase = INTRO; introStart = millis(); lastInput = millis();
    charge = 0; affiche = 0; flash = 0; special = false;
}

void dice_input(int sens) {
    lastInput = millis();
    if (phase == INTRO) phase = ATTRACT;        // a gesture skips the intro

    if (sens > 0) {                             // clockwise : fill the ring
        if (enContact()) return;                // in Contact we don't roll
        if (millis() - tRoll < COOLDOWN_MS) return;       // small delay after a roll
        if (phase == REVEAL || phase == ATTRACT) {        // we (re)start from an empty ring
            if (phase == ATTRACT) attractLeft = millis();
            phase = CHARGE; charge = 0; affiche = 0;
        }
        charge += pas();
        if (charge >= 100) { charge = 100; lanceReveal(); }
    } else if (sens < 0) {                       // counter-clockwise : change category
        indexN = (indexN + 1) % (N_COUNT + 1);   // ... 99 -> Contact -> 2 ...
    }
}

void dice_update() {
    if (phase == INTRO) {
        if (millis() - introStart >= INTRO_MS) versAttente();
        return;
    }

    if (phase == CHARGE) {
        // the display smoothly follows the target
        if (affiche < charge) { affiche += FILL_SPEED; if (affiche > charge) affiche = charge; }
        if (affiche > charge) { affiche -= FILL_SPEED; if (affiche < charge) affiche = charge; }

        // stop around 2/3 long enough -> opens the hidden menu (67)
        if (charge >= SECRET_LO && charge <= SECRET_HI) {
            if (zoneDebut == 0) zoneDebut = millis();
            if (millis() - zoneDebut >= SECRET_MS) lanceSecret();
        } else {
            zoneDebut = 0;
        }

        // ring empty for 10 s -> back to idle
        if (charge == 0 && millis() - lastInput >= ATTRACT_MS) versAttente();

    } else if (phase == REVEAL) {
        if (millis() - revDebut >= (uint32_t)revMs) {
            if (special) versAttente();          // the 67 goes back to idle on its own
            else { phase = CHARGE; charge = 0; affiche = 0; zoneDebut = 0; }
        }
    }

    if (flash > 0) flash -= 5;                   // the flash fades out
}

DiceView dice_view() {
    DiceView v;
    v.nombre   = nombre;
    v.n        = N_VALUES[indexN < N_COUNT ? indexN : 0];
    v.flash    = flash < 0 ? 0 : flash;
    v.special  = special;
    v.hehe     = special && (millis() - specialStart < HEHE_MS);
    v.contact  = enContact();
    v.intro    = (phase == INTRO);
    v.attract  = (phase == ATTRACT);
    v.enReveal = (phase == REVEAL);

    v.introProg = v.intro ? (int)((millis() - introStart) * 100 / INTRO_MS) : 0;
    if (v.introProg > 100) v.introProg = 100;

    if (v.enReveal) {
        uint32_t e = millis() - revDebut;
        v.reveal = revMs > 0 ? (int)(e * 100 / (uint32_t)revMs) : 100;
        if (v.reveal > 100) v.reveal = 100;
    } else v.reveal = 0;
    v.charge = v.enReveal ? 100 : affiche;

    v.dwell = (zoneDebut && phase == CHARGE)
                  ? (int)((millis() - zoneDebut) * 100 / SECRET_MS) : 0;
    if (v.dwell > 100) v.dwell = 100;

    v.montreNombre = montre;
    v.attractAge   = (int)(millis() - attractStart);           // keeps going even after (smooth fade)
    if (v.attractAge < 0) v.attractAge = 0;

    // Tips : soft fade-in on arrival, FAST fade-out as soon as you turn.
    if (phase == ATTRACT)                            v.attractFade = (int)((millis() - attractStart) * 100 / FADE_MS);
    else if (millis() - attractLeft < ATTRACT_OUT_MS) v.attractFade = 100 - (int)((millis() - attractLeft) * 100 / ATTRACT_OUT_MS);
    else                                             v.attractFade = 0;
    if (v.attractFade > 100) v.attractFade = 100; if (v.attractFade < 0) v.attractFade = 0;

    // The number fades just before going idle, or at the end of the secret 67.
    v.numFade = 100;
    if (phase == CHARGE && charge == 0) {
        uint32_t idle = millis() - lastInput;
        if (idle > ATTRACT_MS - FADE_MS) v.numFade = (int)((ATTRACT_MS - idle) * 100 / FADE_MS);
    }
    if (phase == REVEAL && special) {
        uint32_t left = (uint32_t)revMs - (millis() - revDebut);
        if (left < FADE_MS) v.numFade = (int)(left * 100 / FADE_MS);
    }
    if (v.numFade > 100) v.numFade = 100; if (v.numFade < 0) v.numFade = 0;
    return v;
}
