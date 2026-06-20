#pragma once

#include <Arduino.h>

// ============================================================
// Librairie encodeur rotatif mecanique.
// par Merlin Racine 2026 (@merlin-rce)
// ============================================================
//
// Ce que ca fait :
//   Tu tournes le bouton -> read() dit dans quel sens.
//   Un cran = un pas.   +1 = horaire,  -1 = anti-horaire,  0 = rien.
//
// Comment ca marche :
//   Les deux lignes A/B sont lues sur interruption et decodees avec la table
//   de quadrature standard. Chaque front est capture (le spin rapide n'est
//   jamais rate) et les transitions impossibles/rebonds sont rejetees.
//   read() est non-bloquant : il sort juste un cran a la fois.
//
// Utilisation :
//   Encoder enc(6, 5);   // broche A = 6, broche B = 5
//   enc.begin();         // une fois dans setup()
//   int pas = enc.read();   // dans loop(), aussi souvent que tu veux
//
// Cablage :
//   Broche A et broche B sur l'encodeur, la 3eme (commune) a GND.
//   Pas de resistances externes (pull-ups internes utilises).
//
// Plusieurs encodeurs : fais plusieurs objets, chacun ses broches.
// ============================================================

class Encoder {
public:
  // debounceMs garde pour compatibilite ; la quadrature le rend inutile (ignore).
  Encoder(uint8_t pinA, uint8_t pinB, unsigned long debounceMs = 0);

  // A appeler une fois dans setup(). Configure les broches et les interruptions.
  void begin();

  // A appeler le plus souvent possible dans loop().
  // Retourne +1 (horaire), -1 (anti-horaire) ou 0 (rien). Un cran par appel.
  int read();

private:
  uint8_t          _pinA;
  uint8_t          _pinB;
  volatile int32_t _count;     // comptes bruts de quadrature (DETENT par cran)
  volatile uint8_t _state;     // dernier etat (A<<1 | B)
  portMUX_TYPE     _mux;       // pour proteger _count et _state dans les interruptions

  void IRAM_ATTR handle();      // appele a chaque front sur A ou B
  static void IRAM_ATTR isr(void* arg); // trampoline pour appeler handle() avec le bon "this"
};
