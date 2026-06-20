#pragma once

// DiceView = une "photo" de l'etat du jeu a un instant, que l'affichage lit.
// dice.cpp calcule tout ca, ui.cpp ne fait que dessiner.
struct DiceView {
    int  nombre;      // le nombre affiche
    int  n;           // la categorie courante ("1 in N")
    int  charge;      // 0..100 : remplissage de l'anneau
    int  reveal;      // 0..100 : avancement de l'animation de tirage
    int  dwell;       // 0..100 : temps passe dans la zone secrete (2/3)
    int  flash;       // 0..100 : flash blanc au tirage
    int  introProg;   // 0..100 : avancement de l'intro
    int  numFade;     // 0..100 : opacite du nombre (pour les transitions)
    int  attractFade; // 0..100 : opacite des conseils (pour les transitions)
    int  attractAge;  // depuis quand on est au repos (ms) -> choisit le conseil
    bool montreNombre;// vrai s'il y a un nombre a montrer (apres un tirage)
    bool intro;       // l'intro joue
    bool attract;     // au repos : pas de nombre, on montre des conseils
    bool enReveal;    // l'animation de tirage joue
    bool special;     // le 67 secret (menu cache)
    bool hehe;        // afficher le petit "hehe you found it"
    bool contact;     // categorie Contact : on affiche le QR code
};

void dice_begin();
void dice_input(int sens);   // +1 horaire (remplir), -1 anti-horaire (changer de categorie)
void dice_update();
DiceView dice_view();
