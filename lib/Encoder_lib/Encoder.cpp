#include "Encoder.h"

Encoder::Encoder(uint8_t pinA, uint8_t pinB, unsigned long debounceMs)
  : _pinA(pinA),
    _pinB(pinB),
    _debounceMs(debounceMs),
    _lastA(HIGH),
    _waitingRelease(false),
    _edgeTime(0) {}

void Encoder::begin()
{
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
}

int Encoder::read()
{
  int delta = 0;
  int a = digitalRead(_pinA);

  if (!_waitingRelease && _lastA == HIGH && a == LOW) {
    _edgeTime = millis();
    _waitingRelease = true;
  }

  if (_waitingRelease && millis() - _edgeTime >= _debounceMs) {
    if (digitalRead(_pinA) == HIGH) {
      delta = (digitalRead(_pinB) == LOW) ? +1 : -1;
      _waitingRelease = false;
    }
  }

  _lastA = a;
  return delta;
}
