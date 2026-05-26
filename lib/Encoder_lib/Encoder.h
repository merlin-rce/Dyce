#pragma once

#include <Arduino.h>

// ============================================================
// Encoder simple library for mechanical rotary encoders.
// made by Merlin Racine 2026 (@merlin-rce)
// ============================================================
//
// What it does:
//   You turn the knob -> Encoder.read() tells you which way it moved.
//   One click = one step
//  +1 = CW,  -1 = CCW,  0 = nothing.
//
// How to use it:
//
//   0) Drop the [Encoder_lib] folder in your PIO `lib/` folder,
//      then:  #include <Encoder.h>
//
//   1) Create an Encoder with the two pins you wired:
//      exemple:  Encoder encoder(6, 5);   // pin A = 6, pin B = 5
//
//   2) In setup(), call begin() once:
//        encoder.begin();
//
//   3) In loop(), call read() as often as you can. It is
//      non-blocking, so it plays nice with TFT, wifi, etc.
//        int step = encoder.read();
//        if (step > 0) { /* clockwise */ }
//        if (step < 0) { /* counter-clockwise */ }
//
// Wiring:
//   Pin A and Pin B to the encoder, the third pin to GND.
//   No external resistors (internal pull-ups are used).
//
// Multiple encoders:
//
//   Just make more objects, each with its own pins. That's
//   the beauty of classes.
//     Encoder encoderClock(6, 5);
//     Encoder encoderMenu(3, 4);
//
// ============================================================

class Encoder {
public:

  // = how long to wait for the contact to settle.
  // debounceMs (2 ms) works for most cheap encoders. can be set higher if you have a noisy encoder.
  Encoder(uint8_t pinA, uint8_t pinB, unsigned long debounceMs = 2);

  // Call once in setup(). Configures the pins.
  void begin();

  // Call as often as possible in loop().
  // Returns +1 (CW), -1 (CCW), or 0 (nothing).
  int read();

private:
  uint8_t _pinA;
  uint8_t _pinB;
  unsigned long _debounceMs;
  int _lastA;
  bool _waitingRelease;
  unsigned long _edgeTime;
};
