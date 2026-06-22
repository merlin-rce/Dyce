#pragma once

#include <Arduino.h>

// ============================================================
// Mechanical rotary encoder library.
// by Merlin Racine 2026 (@merlin-rce)
// ============================================================
//
// What it does :
//   You turn the knob -> read() tells which way.
//   One notch = one step.   +1 = clockwise,  -1 = counter-clockwise,  0 = nothing.
//
// How it works :
//   The two A/B lines are read on interrupt and decoded with the standard
//   quadrature table. Every edge is captured (a fast spin is never
//   missed) and impossible transitions/bounces are rejected.
//   read() is non-blocking : it just outputs one notch at a time.
//
// Usage :
//   Encoder enc(6, 5);   // pin A = 6, pin B = 5
//   enc.begin();         // once in setup()
//   int pas = enc.read();   // in loop(), as often as you want
//
// Wiring :
//   Pin A and pin B on the encoder, the 3rd (common) to GND.
//   No external resistors (internal pull-ups used).
//
// Multiple encoders : make several objects, each with its own pins.
// ============================================================

class Encoder {
public:
  // debounceMs kept for compatibility ; quadrature makes it useless (ignored).
  Encoder(uint8_t pinA, uint8_t pinB, unsigned long debounceMs = 0);

  // Call once in setup(). Sets up the pins and the interrupts.
  void begin();

  // Call as often as possible in loop().
  // Returns +1 (clockwise), -1 (counter-clockwise) or 0 (nothing). One notch per call.
  int read();

private:
  uint8_t          _pinA;
  uint8_t          _pinB;
  volatile int32_t _count;     // raw quadrature counts (DETENT per notch)
  volatile uint8_t _state;     // last state (A<<1 | B)
  portMUX_TYPE     _mux;       // to protect _count and _state inside the interrupts

  void IRAM_ATTR handle();      // called on every edge on A or B
  static void IRAM_ATTR isr(void* arg); // trampoline to call handle() with the right "this"
};
