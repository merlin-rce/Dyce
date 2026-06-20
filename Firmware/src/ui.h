#pragma once
#include "dice.h"   // pour DiceView

// Affichage sur l'ecran rond (anneau de charge). Tout dans un sprite plein ecran.
void ui_begin();
void ui_draw(const DiceView& v);
