#include <Arduino.h>
#include <Encoder.h>
#include "config.h"
#include "rng.h"
#include "ui.h"
#include "dice.h"

// ===== DYCE =====
// Anneau de charge : tu spin pour remplir l'anneau, au sommet ca tire un nombre.
// Sens inverse : change la rarete N. Pause vers 2/3 : arme un nombre special.
// Tout est non-bloquant (millis), 100% hors-ligne.

Encoder enc(PIN_ENC_A, PIN_ENC_B);   // encodeur en anneau

void setup() {
    Serial.begin(115200);     // hasard materiel (esp_random)
    ui_begin(SCREEN_ROTATION);       // ecran + sprite plein ecran
    enc.begin();
    dice_begin();     // premier tirage + intro
}

void loop() {
    int sens = enc.read() * ENC_DIR;
    if (sens != 0) dice_input(sens);

    static uint32_t t = 0;
    if (millis() - t >= FRAME_MS) {
        t = millis();
        dice_update();
        ui_draw(dice_view());
    }
}
