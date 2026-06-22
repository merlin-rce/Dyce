#pragma once

// DiceView = a "snapshot" of the game state at one moment, that the display reads.
// dice.cpp computes all this, ui.cpp only draws.
struct DiceView {
    int  nombre;      // the displayed number
    int  n;           // the current category ("1 in N")
    int  charge;      // 0..100 : ring fill
    int  reveal;      // 0..100 : progress of the roll animation
    int  dwell;       // 0..100 : time spent in the secret zone (2/3)
    int  flash;       // 0..100 : white flash on roll
    int  introProg;   // 0..100 : intro progress
    int  numFade;     // 0..100 : number opacity (for transitions)
    int  attractFade; // 0..100 : tips opacity (for transitions)
    int  attractAge;  // how long we've been idle (ms) -> picks the tip
    bool montreNombre;// true if there's a number to show (after a roll)
    bool intro;       // the intro is playing
    bool attract;     // idle : no number, we show tips
    bool enReveal;    // the roll animation is playing
    bool special;     // the secret 67 (hidden menu)
    bool hehe;        // show the little "hehe you found it"
    bool contact;     // Contact category : we show the QR code
};

void dice_begin();
void dice_input(int sens);   // +1 clockwise (fill), -1 counter-clockwise (change category)
void dice_update();
DiceView dice_view();
