#pragma once
#include "dice.h"   // for DiceView

// Display on the round screen (charge ring). Everything in a fullscreen sprite.
void ui_begin(int Rotation);   // init screen + fullscreen sprite
void ui_draw(const DiceView& v);
