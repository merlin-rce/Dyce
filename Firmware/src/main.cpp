#include <Arduino.h>
#include <Encoder.h>
#include "config.h"
#include "rng.h"
#include "ui.h"
#include "dice.h"

// ===== DYCE =====
// Charge ring : you spin to fill the ring, at the top it rolls a number.
// Other way : changes the rarity N. Pause around 2/3 : arms a special number.
// Everything is non-blocking (millis), 100% offline.

Encoder enc(PIN_ENC_A, PIN_ENC_B);   // ring encoder

void setup() {
    Serial.begin(115200);     // hardware randomness (esp_random)
    ui_begin(SCREEN_ROTATION);       // screen + fullscreen sprite
    enc.begin();
    dice_begin();     // first roll + intro
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
