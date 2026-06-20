#include "dice.h"
#include "config.h"
#include "rng.h"
#include <Arduino.h>

// Le jeu passe par 4 phases :
//   INTRO    : l'animation de demarrage
//   ATTRACT  : au repos (pas de nombre), on montre des conseils
//   CHARGE   : on tourne pour remplir l'anneau
//   REVEAL   : l'anneau est plein -> on montre le nombre tire
enum { INTRO, ATTRACT, CHARGE, REVEAL };

static int      nombre   = 7;
static int      indexN   = N_START;   // 0..N_COUNT ; N_COUNT = categorie Contact
static int      charge   = 0;         // remplissage vise 0..100
static int      affiche  = 0;         // remplissage montre (suit "charge" en douceur)
static int      flash    = 0;
static int      phase    = INTRO;
static bool     special  = false;     // le 67 secret
static bool     montre   = false;     // y a-t-il un nombre a afficher ?
static uint32_t introStart   = 0;
static uint32_t revDebut     = 0;
static int      revMs        = 0;     // duree de la phase REVEAL en cours
static uint32_t tRoll        = 0;     // moment du dernier tirage (pour le cooldown)
static uint32_t zoneDebut    = 0;     // depuis quand on est dans la zone 2/3 (0 = dehors)
static uint32_t lastInput    = 0;     // derniere action sur l'encodeur
static uint32_t attractStart = 0;     // moment ou on est passe en attente
static uint32_t attractLeft  = 0;     // moment ou on a quitte l'attente
static uint32_t specialStart = 0;     // moment ou le 67 secret est apparu

static bool enContact() { return indexN == N_COUNT; }   // derniere categorie = QR

// Taille du pas : grand quand l'anneau est bas, petit quand il est haut.
static int pas() {
    int p = STEP_MIN_INC + (STEP_MAX_INC - STEP_MIN_INC) * (100 - charge) / 100;
    p += (int)rng_roll(3) - 2;          // un peu d'irregularite
    return p < 1 ? 1 : p;
}

// Retour au repos (efface le nombre, lance le fondu des conseils).
static void versAttente() {
    phase = ATTRACT; attractStart = millis();
    special = false; montre = false; charge = 0; affiche = 0;
}

// Tirage normal quand l'anneau est plein.
static void lanceReveal() {
    nombre  = rng_roll(N_VALUES[indexN]);
    special = false; montre = true; revMs = REVEAL_MS;
    phase = REVEAL; revDebut = millis(); tRoll = millis();
    flash = 100; zoneDebut = 0;
}

// Menu cache : on l'ouvre en s'arretant vers 2/3 (pas besoin de finir l'anneau).
// Il montre 67 pendant 6 s puis revient tout seul au repos.
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
    if (phase == INTRO) phase = ATTRACT;        // un geste saute l'intro

    if (sens > 0) {                             // horaire : remplir l'anneau
        if (enContact()) return;                // dans Contact on ne tire pas
        if (millis() - tRoll < COOLDOWN_MS) return;       // petite tempo apres un tirage
        if (phase == REVEAL || phase == ATTRACT) {        // on (re)part d'un anneau vide
            if (phase == ATTRACT) attractLeft = millis();
            phase = CHARGE; charge = 0; affiche = 0;
        }
        charge += pas();
        if (charge >= 100) { charge = 100; lanceReveal(); }
    } else if (sens < 0) {                       // anti-horaire : changer de categorie
        indexN = (indexN + 1) % (N_COUNT + 1);   // ... 99 -> Contact -> 2 ...
    }
}

void dice_update() {
    if (phase == INTRO) {
        if (millis() - introStart >= INTRO_MS) versAttente();
        return;
    }

    if (phase == CHARGE) {
        // l'affichage suit la cible en douceur
        if (affiche < charge) { affiche += FILL_SPEED; if (affiche > charge) affiche = charge; }
        if (affiche > charge) { affiche -= FILL_SPEED; if (affiche < charge) affiche = charge; }

        // s'arreter vers 2/3 assez longtemps -> ouvre le menu cache (67)
        if (charge >= SECRET_LO && charge <= SECRET_HI) {
            if (zoneDebut == 0) zoneDebut = millis();
            if (millis() - zoneDebut >= SECRET_MS) lanceSecret();
        } else {
            zoneDebut = 0;
        }

        // anneau vide depuis 10 s -> retour au repos
        if (charge == 0 && millis() - lastInput >= ATTRACT_MS) versAttente();

    } else if (phase == REVEAL) {
        if (millis() - revDebut >= (uint32_t)revMs) {
            if (special) versAttente();          // le 67 revient seul au repos
            else { phase = CHARGE; charge = 0; affiche = 0; zoneDebut = 0; }
        }
    }

    if (flash > 0) flash -= 5;                   // le flash s'estompe
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
    v.attractAge   = (int)(millis() - attractStart);           // continue meme apres (fondu lisse)
    if (v.attractAge < 0) v.attractAge = 0;

    // Conseils : apparition douce a l'arrivee, disparition RAPIDE des qu'on tourne.
    if (phase == ATTRACT)                            v.attractFade = (int)((millis() - attractStart) * 100 / FADE_MS);
    else if (millis() - attractLeft < ATTRACT_OUT_MS) v.attractFade = 100 - (int)((millis() - attractLeft) * 100 / ATTRACT_OUT_MS);
    else                                             v.attractFade = 0;
    if (v.attractFade > 100) v.attractFade = 100; if (v.attractFade < 0) v.attractFade = 0;

    // Le nombre se fond juste avant de partir au repos, ou a la fin du 67 secret.
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
