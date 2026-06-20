#pragma once

// ===== Reglages DYCE =====

// Broches encodeur
#define PIN_ENC_A 6
#define PIN_ENC_B 5
#define ENC_DIR   1        // sens de rotation (-1 pour inverser)

// Categories "1 in N" (anti-horaire pour changer). Apres la derniere valeur
// vient la categorie "Contact" (index = N_COUNT) qui affiche un QR code.
static const int N_VALUES[] = { 2, 5, 10, 50, 99 };
static const int N_COUNT    = 5;
#define N_START 1          // demarre sur le 5

// Remplissage de l'anneau : gros pas en bas, petits en haut -> peu de crans
#define STEP_MAX_INC 28
#define STEP_MIN_INC 10
#define FILL_SPEED   8     // vitesse de remplissage affiche
#define FRAME_MS     33    // rafraichissement (~30 fps)
#define COOLDOWN_MS  250   // mini tempo apres un tirage (spam ok, mais pas a l'exces)

// Reveal : anim puis le nombre reste un peu, puis ca s'efface
#define REVEAL_MS  2500

// Secret : s'arreter vers 2/3 -> tombe sur 67
#define SECRET_LO 55
#define SECRET_HI 80
#define SECRET_MS 1500

// Intro de boot + mode attente
#define INTRO_MS   4500    // intro film, rythmee
#define ATTRACT_MS 10000   // sans rien toucher -> efface le nombre, retour aux conseils
#define FADE_MS    900     // apparition douce des conseils
#define ATTRACT_OUT_MS 250 // disparition rapide des conseils quand on commence a tourner
#define FIRST_MS   8000    // le tout 1er conseil reste plus longtemps
#define HEHE_MS    6000    // le 67 secret reste affiche ce temps puis disparait
