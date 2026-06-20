#include "ui.h"
#include <Arduino.h>
#include <string.h>
#include <TFT_eSPI.h>
#include "qr_image.h"               // ta vraie image QR (categorie Contact)
#include "logo.h"                   // ton logo complet (intro)
#include "icon.h"                   // juste le "D" (mode attente)
#include "config.h"
#include "fonts/Google_Sans_Flex_9pt_SemiBold_150.h"
// FreeSansBold 9/12/18/24pt sont fournies dans TFT_eSPI

static TFT_eSPI    tft = TFT_eSPI();
static TFT_eSprite spr = TFT_eSprite(&tft);

// couleurs (RGB565) : gris + bleu marine
static const uint16_t FOND   = 0x1149;   // bleu marine (fond)
static const uint16_t RAIL   = 0x2945;   // anneau de fond (toujours visible)
static const uint16_t TEXTE  = 0xC638;   // gris clair mat
static const uint16_t GRIS   = 0x8410;   // texte secondaire
static const uint16_t OR     = 0xFE60;   // accent (easter egg)
static const uint16_t ACCENT = 0x4DD9;   // bleu doux (intro)

// melange deux couleurs RGB565 (t = 0..100 : 0 = a, 100 = b). Sert a tout faire
// apparaitre/disparaitre en douceur.
static uint16_t melange(uint16_t a, uint16_t b, int t) {
    if (t < 0) t = 0; if (t > 100) t = 100;
    int ar=(a>>11)&0x1F, ag=(a>>5)&0x3F, ab=a&0x1F;
    int br=(b>>11)&0x1F, bg=(b>>5)&0x3F, bb=b&0x1F;
    return ((ar+(br-ar)*t/100)<<11) | ((ag+(bg-ag)*t/100)<<5) | (ab+(bb-ab)*t/100);
}

// une teinte vive a partir d'un angle h (0..359) -> couleur arc-en-ciel
static uint16_t teinte(int h) {
    h = ((h % 360) + 360) % 360;
    int x = (h % 60) * 255 / 60, r, g, b;
    switch (h / 60) {
        case 0: r=255; g=x;     b=0;     break;
        case 1: r=255-x; g=255; b=0;     break;
        case 2: r=0;   g=255;   b=x;     break;
        case 3: r=0;   g=255-x; b=255;   break;
        case 4: r=x;   g=0;     b=255;   break;
        default:r=255; g=0;     b=255-x; break;
    }
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// dessine un gros point de l'anneau a l'angle "deg" (0 = haut, sens horaire)
static void point(int deg, int rayon, int taille, uint16_t c) {
    float a = (deg - 90) * 0.0174533f;
    spr.fillCircle(120 + (int)(cosf(a) * rayon), 120 + (int)(sinf(a) * rayon), taille, c);
}

// petite comete grise qui glisse le long de l'anneau (montre le sens du spin)
static void comete(uint32_t ms, int fade) {
    int tete = (ms / 18) % 360;            // un peu plus rapide qu'avant
    for (int i = 0; i < 8; i++) {          // une tete + 7 points qui s'estompent
        int b = (55 - i * 7) * fade / 100; if (b < 0) b = 0;
        point(tete - i * 4, 113, 8, melange(RAIL, TEXTE, b));
    }
}

// renvoie une opacite 0..100 qui monte au debut et descend a la fin d'un segment
static int fondu(int prog, int s, int e, int f) {
    if (prog <= s || prog >= e) return 0;
    int a = 100;
    if (prog < s + f) a = (prog - s) * 100 / f;
    if (prog > e - f) a = (e - prog) * 100 / f;
    return a < 0 ? 0 : (a > 100 ? 100 : a);
}

// texte centre, coupe en 2 lignes s'il y a un '\n'
static void texteCentre(const char* s, int cx, int cy, uint16_t c) {
    spr.setTextColor(c);
    const char* nl = strchr(s, '\n');
    if (!nl) { spr.drawString(s, cx, cy); return; }
    char l1[24]; int k = nl - s; if (k > 23) k = 23;
    memcpy(l1, s, k); l1[k] = 0;
    spr.drawString(l1,    cx, cy - 15);
    spr.drawString(nl + 1, cx, cy + 15);
}

// petit drapeau suisse s x s (l'emoji n'existe pas dans la police)
static void drapeauCH(int cx, int cy, int s, int a) {
    spr.fillRect(cx - s/2, cy - s/2, s, s, melange(FOND, 0xF800, a));
    int t = s / 5 + 1;
    spr.fillRect(cx - t/2, cy - s/3, t, 2*s/3, melange(FOND, 0xFFFF, a));
    spr.fillRect(cx - s/3, cy - t/2, 2*s/3, t, melange(FOND, 0xFFFF, a));
}

// blit une image (le fond marine est deja integre dedans), avec un fondu 0..100
static void blit(const unsigned short* img, int w, int h, int ox, int oy, int alpha) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint16_t c = img[y * w + x];
            if (alpha < 100) c = melange(FOND, c, alpha);
            spr.drawPixel(ox + x, oy + y, c);
        }
}
static void drawLogo(int ox, int oy, int alpha) { blit(LOGO, LOGO_W, LOGO_H, ox, oy, alpha); }
static void drawIcon(int ox, int oy, int alpha) { blit(ICON, ICON_W, ICON_H, ox, oy, alpha); }

// dessine l'anneau gris de fond (toujours present) + la partie remplie
static void dessineAnneau(const DiceView& v, uint32_t ms, int pulse, bool rainbow) {
    for (int a = 0; a < 360; a += 3) point(a, 113, 8, RAIL);          // anneau gris toujours la
    int fin = v.enReveal ? 360 : v.charge * 360 / 100;
    int retour = (v.enReveal && !v.special) ? (v.reveal * 4 > 100 ? 100 : v.reveal * 4) : 0; // se fond vers le gris
    uint16_t flashCol = v.special ? teinte(ms / 3) : 0xFFFF;
    for (int a = 0; a < fin; a += 3) {
        uint16_t c;
        if (rainbow) c = teinte(a * 2 + ms / 3);                      // 67 : arc-en-ciel
        else {
            c = teinte(160 + a / 2 + ms / 25);                        // joli degrade anime
            if (v.dwell > 0) c = melange(c, teinte(a * 3 + ms / 2), v.dwell);  // scintille pres du secret
        }
        c = melange(c, 0xFFFF, pulse);
        c = melange(c, flashCol, v.flash);
        c = melange(c, RAIL, retour);                                 // revient au gris (pas de "trou")
        point(a, 113, 8, c);
    }
}

// intro film : "merlin-rce" -> presents -> logo DYCE -> fondu vers l'app
static void dessineIntro(int prog, uint32_t ms) {
    spr.fillSprite(FOND);
    spr.setTextDatum(MC_DATUM);

    // 1) "merlin-rce" : lettres en fondu, de gauche a droite (lisse, proportionnel)
    {
        spr.setFreeFont(&FreeSansBold18pt7b);
        spr.setTextDatum(TL_DATUM);
        const char* nom = "merlin-rce";
        int x = 120 - spr.textWidth(nom) / 2;
        int sortie = (prog <= 30) ? 100 : (38 - prog) * 100 / 8;      // fondu de sortie du segment
        if (sortie < 0) sortie = 0;
        for (int i = 0; nom[i]; i++) {
            char s[2] = { nom[i], 0 };
            int la = (prog - 3 - i) * 100 / 10;                       // chaque lettre apparait apres la precedente
            if (la < 0) la = 0; if (la > 100) la = 100;
            spr.setTextColor(melange(FOND, TEXTE, la * sortie / 100));
            spr.drawString(s, x, 94);
            x += spr.textWidth(s);
        }
        int af = fondu(prog, 14, 38, 8);
        if (af > 0) drapeauCH(120, 150, 18, af);
        spr.setTextDatum(MC_DATUM);
    }

    // 2) "presents" (discret)
    int a2 = fondu(prog, 40, 62, 8);
    if (a2 > 0) {
        spr.setFreeFont(&FreeSansBold12pt7b);
        spr.setTextColor(melange(FOND, GRIS, a2));
        spr.drawString("presents", 120, 120);
    }

    // 3) le LOGO : apparait puis se fond a la fin -> transition douce vers l'app
    if (prog > 64) {
        int a3 = 100;
        if (prog < 78) a3 = (prog - 64) * 100 / 14;                   // apparition
        if (prog > 92) a3 = (100 - prog) * 100 / 8;                   // disparition (vers l'app)
        if (a3 < 0) a3 = 0;
        drawLogo(120 - LOGO_W / 2, 110 - LOGO_H / 2, a3);
    }
}

// categorie Contact : fond blanc + ta VRAIE image QR (avec l'octocat) au centre
static void dessineContact() {
    spr.fillSprite(0xFFFF);                                           // fond blanc plein ecran
    int ox = 120 - QR_IMG_W / 2, oy = 120 - QR_IMG_H / 2;
    int bpr = (QR_IMG_W + 7) / 8;                                     // octets par ligne (bit=1 -> noir)
    for (int y = 0; y < QR_IMG_H; y++)
        for (int x = 0; x < QR_IMG_W; x++)
            if (QR_IMG[y * bpr + (x >> 3)] & (0x80 >> (x & 7)))
                spr.drawPixel(ox + x, oy + y, 0x0000);
}

// conseils du mode attente (2 lignes, clairs)
static const char* CONSEILS[] = {
    "Spin to roll\nThe Dice",
    "Find\nThe hidden 67",
    "Turn left to\nChange Dice",
    "Bored ?\nSpin the Dice",
};
static const int NB_CONSEILS = 4;

void ui_begin() {
    tft.init();
    tft.setRotation(0);
    spr.setColorDepth(16);
    spr.createSprite(240, 240);                                       // tampon plein ecran (PSRAM)
}

void ui_draw(const DiceView& v) {
    uint32_t ms = millis();

    // ----- ecrans pleins (priment sur le reste) -----
    if (v.intro)   { dessineIntro(v.introProg, ms); spr.pushSprite(0, 0); return; }
    if (v.contact) { dessineContact();              spr.pushSprite(0, 0); return; }

    char texte[8];   snprintf(texte,  sizeof(texte),  "%d", v.nombre);
    char chance[16]; snprintf(chance, sizeof(chance), "1 in %d", v.n);
    bool rainbow = v.special;                                         // 67 = anneau arc-en-ciel

    spr.fillSprite(melange(FOND, v.special ? teinte(ms / 3) : 0xFFFF, v.flash));  // flash plein ecran

    // petit tremblement + pulsation quand on approche du sommet
    int dx = 0, dy = 0, pulse = 0;
    if (!v.enReveal && v.charge > 65) {
        int t = v.charge - 65, amp = 1 + t / 12;
        dx = (int)(amp * sinf(ms / 22.0f));
        dy = (int)(amp * sinf(ms / 19.0f));
        pulse = (int)((20 + 20 * sinf(ms / 55.0f)) * t / 35);
    }

    dessineAnneau(v, ms, pulse, rainbow);
    spr.setTextDatum(MC_DATUM);

    // ----- le nombre (seulement s'il y en a un a montrer, jamais pendant la
    //        montee de l'anneau qui part du repos -> transition propre) -----
    if (!v.attract && v.montreNombre) {
        int by = (v.enReveal && v.reveal < 12) ? -(12 - v.reveal) : 0;
        if (v.special) {                          // menu cache : tu as trouve le 67
            spr.setTextColor(melange(FOND, teinte(ms / 3 + 180), v.numFade));
            spr.setFreeFont(&Google_Sans_Flex_9pt_SemiBold_150);
            spr.drawString("67", 120 + dx, 100 + dy + by);
            if (v.hehe) {
                spr.setFreeFont(&FreeSansBold9pt7b);
                spr.setTextColor(melange(FOND, OR, v.numFade));
                spr.drawString("hehe you found it", 120, 178);
            }
        } else {                                  // nombre normal + categorie
            spr.setTextColor(melange(FOND, TEXTE, v.numFade));
            spr.setFreeFont(&Google_Sans_Flex_9pt_SemiBold_150);
            spr.drawString(texte, 120 + dx, 100 + dy + by);
            spr.setFreeFont(&FreeSansBold12pt7b);
            spr.setTextColor(melange(FOND, GRIS, v.numFade));
            spr.drawString(chance, 120, 182);
        }
    }

    // ----- au repos : juste l'icone "D" en haut, et l'ADVICE en gros (element principal).
    //        Le 1er ("Spin to roll") reste plus longtemps. -----
    if (v.attractFade > 0) {
        drawIcon(120 - ICON_W / 2, 40, v.attractFade);   // le "D" seul, en haut
        int age = v.attractAge, idx, t, per;
        if (age < FIRST_MS) { idx = 0;                            per = FIRST_MS; t = age; }
        else { int k = (age - FIRST_MS) / 5000; idx = (k + 1) % NB_CONSEILS; per = 5000; t = (age - FIRST_MS) % 5000; }
        int br = 100;
        if (t < 700)            br = t * 100 / 700;
        else if (t > per - 700) br = (per - t) * 100 / 700;
        spr.setFreeFont(&FreeSansBold12pt7b);            // advice = element principal
        texteCentre(CONSEILS[idx], 120, 130, melange(FOND, TEXTE, br * v.attractFade / 100));
    }

    spr.pushSprite(0, 0);
}
