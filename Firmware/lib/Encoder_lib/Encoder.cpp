#include "Encoder.h"
#include "driver/gpio.h"

// One notch of a classic mechanical encoder = one full cycle = 4 edges.
// Set 2 if your encoder needs two clicks for one step.
static const int DETENT = 4;

// Standard quadrature table.
// index = (previous state << 2) | current state, value = direction
// (0 = impossible transition -> rejects bounces).
static const int8_t QTABLE[16] = {
   0, +1, -1,  0,
  -1,  0,  0, +1,
  +1,  0,  0, -1,
   0, -1, +1,  0
};

Encoder::Encoder(uint8_t pinA, uint8_t pinB, unsigned long /*debounceMs*/)
  : _pinA(pinA), _pinB(pinB), _count(0), _state(0),
    _mux(portMUX_INITIALIZER_UNLOCKED) {}

void Encoder::begin()
{
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  _state = (uint8_t)((digitalRead(_pinA) << 1) | digitalRead(_pinB));
  attachInterruptArg(digitalPinToInterrupt(_pinA), isr, this, CHANGE);
  attachInterruptArg(digitalPinToInterrupt(_pinB), isr, this, CHANGE);
}

void IRAM_ATTR Encoder::isr(void* arg)
{
  static_cast<Encoder*>(arg)->handle();
}

// called on every edge on A or B
void IRAM_ATTR Encoder::handle()
{
  uint8_t s = (uint8_t)((gpio_get_level((gpio_num_t)_pinA) << 1) |
                         gpio_get_level((gpio_num_t)_pinB));
  int8_t sens = QTABLE[(_state << 2) | s];
  _state = s;
  if (sens) {
    portENTER_CRITICAL_ISR(&_mux);
    _count += sens;
    portEXIT_CRITICAL_ISR(&_mux);
  }
}

int Encoder::read()
{
  int res = 0;
  portENTER_CRITICAL(&_mux);
  if      (_count >=  DETENT) { _count -= DETENT; res = +1; }
  else if (_count <= -DETENT) { _count += DETENT; res = -1; }
  portEXIT_CRITICAL(&_mux);
  return res;
}
