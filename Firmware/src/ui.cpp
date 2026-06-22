#include "ui.h"
#include <Arduino.h>
#include <string.h>
#include <TFT_eSPI.h>
#include "qr_image.h"               // your real QR image (Contact category)
#include "logo.h"                   // your full logo (intro)
#include "icon.h"                   // just the "D" (idle mode)
#include "config.h"
#include "fonts/Google_Sans_Flex_9pt_SemiBold_150.h"
// FreeSansBold 9/12/18/24pt are provided in TFT_eSPI

static TFT_eSPI    tft = TFT_eSPI();
static TFT_eSprite spr = TFT_eSprite(&tft);

// colors (RGB565) : grey + navy blue
static const uint16_t FOND   = 0x1149;   // navy blue (background)
static const uint16_t RAIL   = 0x2945;   // background ring (always visible)
static const uint16_t TEXTE  = 0xC638;   // matte light grey
static const uint16_t GRIS   = 0x8410;   // secondary text
static const uint16_t OR     = 0xFE60;   // accent (easter egg)
static const uint16_t ACCENT = 0x4DD9;   // soft blue (intro)

// blends two RGB565 colors (t = 0..100 : 0 = a, 100 = b). Used to make everything
// fade in/out smoothly.
static uint16_t melange(uint16_t a, uint16_t b, int t) {
    if (t < 0) t = 0; if (t > 100) t = 100;
    int ar=(a>>11)&0x1F, ag=(a>>5)&0x3F, ab=a&0x1F;
    int br=(b>>11)&0x1F, bg=(b>>5)&0x3F, bb=b&0x1F;
    return ((ar+(br-ar)*t/100)<<11) | ((ag+(bg-ag)*t/100)<<5) | (ab+(bb-ab)*t/100);
}

// a vivid hue from an angle h (0..359) -> rainbow color
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

// draws a big dot of the ring at angle "deg" (0 = top, clockwise)
static void point(int deg, int rayon, int taille, uint16_t c) {
    float a = (deg - 90) * 0.0174533f;
    spr.fillCircle(120 + (int)(cosf(a) * rayon), 120 + (int)(sinf(a) * rayon), taille, c);
}

// little grey comet sliding along the ring (shows the spin direction)
static void comete(uint32_t ms, int fade) {
    int tete = (ms / 18) % 360;            // a bit faster than before
    for (int i = 0; i < 8; i++) {          // one head + 7 dots that fade out
        int b = (55 - i * 7) * fade / 100; if (b < 0) b = 0;
        point(tete - i * 4, 113, 8, melange(RAIL, TEXTE, b));
    }
}

// returns an opacity 0..100 that rises at the start and falls at the end of a segment
static int fondu(int prog, int s, int e, int f) {
    if (prog <= s || prog >= e) return 0;
    int a = 100;
    if (prog < s + f) a = (prog - s) * 100 / f;
    if (prog > e - f) a = (e - prog) * 100 / f;
    return a < 0 ? 0 : (a > 100 ? 100 : a);
}

// centered text, split into 2 lines if there's a '\n'
static void texteCentre(const char* s, int cx, int cy, uint16_t c) {
    spr.setTextColor(c);
    const char* nl = strchr(s, '\n');
    if (!nl) { spr.drawString(s, cx, cy); return; }
    char l1[24]; int k = nl - s; if (k > 23) k = 23;
    memcpy(l1, s, k); l1[k] = 0;
    spr.drawString(l1,    cx, cy - 15);
    spr.drawString(nl + 1, cx, cy + 15);
}

// little swiss flag s x s (the emoji doesn't exist in the font)
static void drapeauCH(int cx, int cy, int s, int a) {
    spr.fillRect(cx - s/2, cy - s/2, s, s, melange(FOND, 0xF800, a));
    int t = s / 5 + 1;
    spr.fillRect(cx - t/2, cy - s/3, t, 2*s/3, melange(FOND, 0xFFFF, a));
    spr.fillRect(cx - s/3, cy - t/2, 2*s/3, t, melange(FOND, 0xFFFF, a));
}

// blit an image (the navy background is already baked into it), with a fade 0..100
static void blit(const unsigned short* img, int w, int h, int ox, int oy, int alpha) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint16_t c = img[y * w + x];
            if (alpha < 100) c = melange(FOND, c, alpha);
            spr.drawPixel(ox + x, oy + y, c);
        }
}
static void drawLogo(int ox, int oy, int alpha) { blit(LOGO, LOGO_W, LOGO_H, ox, oy, alpha); }

// blit a resized image (nearest-neighbor), centered on (cx, cy). Used to
// show the little "D" of idle mode more compact, so it doesn't spill
// over the tips text.
static void blitScaled(const unsigned short* img, int w, int h, int dw, int dh, int cx, int cy, int alpha) {
    int ox = cx - dw / 2, oy = cy - dh / 2;
    for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++) {
            uint16_t c = img[(y * h / dh) * w + (x * w / dw)];
            if (alpha < 100) c = melange(FOND, c, alpha);
            spr.drawPixel(ox + x, oy + y, c);
        }
}
// the "D" of idle mode : ~62 % of its size, centered at top (leaves room for the tips)
static void drawAttractIcon(int cx, int cy, int alpha) {
    blitScaled(ICON, ICON_W, ICON_H, (ICON_W * 62) / 100, (ICON_H * 62) / 100, cx, cy, alpha);
}

// draws the grey background ring (always there) + the filled part
static void dessineAnneau(const DiceView& v, uint32_t ms, int pulse, bool rainbow) {
    for (int a = 0; a < 360; a += 3) point(a, 113, 8, RAIL);          // grey ring always there
    int fin = v.enReveal ? 360 : v.charge * 360 / 100;
    int retour = (v.enReveal && !v.special) ? (v.reveal * 4 > 100 ? 100 : v.reveal * 4) : 0; // fades toward grey
    uint16_t flashCol = v.special ? teinte(ms / 3) : 0xFFFF;
    for (int a = 0; a < fin; a += 3) {
        uint16_t c;
        if (rainbow) c = teinte(a * 2 + ms / 3);                      // 67 : rainbow
        else {
            c = teinte(160 + a / 2 + ms / 25);                        // nice animated gradient
            if (v.dwell > 0) c = melange(c, teinte(a * 3 + ms / 2), v.dwell);  // sparkles near the secret
        }
        c = melange(c, 0xFFFF, pulse);
        c = melange(c, flashCol, v.flash);
        c = melange(c, RAIL, retour);                                 // comes back to grey (no "hole")
        point(a, 113, 8, c);
    }
}

// intro movie : "merlin-rce" -> presents -> DYCE logo -> fade to the app
static void dessineIntro(int prog, uint32_t ms) {
    spr.fillSprite(FOND);
    spr.setTextDatum(MC_DATUM);

    // 1) "merlin-rce" : letters fading in, left to right (smooth, proportional)
    {
        spr.setFreeFont(&FreeSansBold18pt7b);
        spr.setTextDatum(TL_DATUM);
        const char* nom = "merlin-rce";
        int x = 120 - spr.textWidth(nom) / 2;
        int sortie = (prog <= 30) ? 100 : (38 - prog) * 100 / 8;      // fade-out of the segment
        if (sortie < 0) sortie = 0;
        for (int i = 0; nom[i]; i++) {
            char s[2] = { nom[i], 0 };
            int la = (prog - 3 - i) * 100 / 10;                       // each letter appears after the previous one
            if (la < 0) la = 0; if (la > 100) la = 100;
            spr.setTextColor(melange(FOND, TEXTE, la * sortie / 100));
            spr.drawString(s, x, 94);
            x += spr.textWidth(s);
        }
        int af = fondu(prog, 14, 38, 8);
        if (af > 0) drapeauCH(120, 150, 18, af);
        spr.setTextDatum(MC_DATUM);
    }

    // 2) "presents" (subtle)
    int a2 = fondu(prog, 40, 62, 8);
    if (a2 > 0) {
        spr.setFreeFont(&FreeSansBold12pt7b);
        spr.setTextColor(melange(FOND, GRIS, a2));
        spr.drawString("presents", 120, 120);
    }

    // 3) the LOGO : appears then fades at the end -> smooth transition to the app
    if (prog > 64) {
        int a3 = 100;
        if (prog < 78) a3 = (prog - 64) * 100 / 14;                   // appear
        if (prog > 92) a3 = (100 - prog) * 100 / 8;                   // disappear (to the app)
        if (a3 < 0) a3 = 0;
        drawLogo(120 - LOGO_W / 2, 110 - LOGO_H / 2, a3);
    }
}

// Contact category : white background + your REAL QR image (with the octocat) in the center
static void dessineContact() {
    spr.fillSprite(0xFFFF);                                           // fullscreen white background
    int ox = 120 - QR_IMG_W / 2, oy = 120 - QR_IMG_H / 2;
    int bpr = (QR_IMG_W + 7) / 8;                                     // bytes per line (bit=1 -> black)
    for (int y = 0; y < QR_IMG_H; y++)
        for (int x = 0; x < QR_IMG_W; x++)
            if (QR_IMG[y * bpr + (x >> 3)] & (0x80 >> (x & 7)))
                spr.drawPixel(ox + x, oy + y, 0x0000);
}

// idle mode tips (2 lines, clear)
static const char* CONSEILS[] = {
    "Spin to roll\nThe Dice",
    "Find\nThe hidden 67",
    "Turn left to\nChange Dice",
    "Bored ?\nSpin the Dice",
};
static const int NB_CONSEILS = 4;

void ui_begin(int Rotation) {
    tft.init();
    tft.setRotation(Rotation);
    spr.setColorDepth(16);
    spr.createSprite(240, 240);                                       // fullscreen buffer (PSRAM)
}

void ui_draw(const DiceView& v) {
    uint32_t ms = millis();

    // ----- full screens (take priority over the rest) -----
    if (v.intro)   { dessineIntro(v.introProg, ms); spr.pushSprite(0, 0); return; }
    if (v.contact) { dessineContact();              spr.pushSprite(0, 0); return; }

    char texte[8];   snprintf(texte,  sizeof(texte),  "%d", v.nombre);
    char chance[16]; snprintf(chance, sizeof(chance), "1 in %d", v.n);
    bool rainbow = v.special;                                         // 67 = rainbow ring

    spr.fillSprite(melange(FOND, v.special ? teinte(ms / 3) : 0xFFFF, v.flash));  // fullscreen flash

    // little shake + pulse when approaching the top
    int dx = 0, dy = 0, pulse = 0;
    if (!v.enReveal && v.charge > 65) {
        int t = v.charge - 65, amp = 1 + t / 12;
        dx = (int)(amp * sinf(ms / 22.0f));
        dy = (int)(amp * sinf(ms / 19.0f));
        pulse = (int)((20 + 20 * sinf(ms / 55.0f)) * t / 35);
    }

    dessineAnneau(v, ms, pulse, rainbow);
    spr.setTextDatum(MC_DATUM);

    // ----- the number (only if there's one to show, never during the
    //        ring rising from idle -> clean transition) -----
    if (!v.attract && v.montreNombre) {
        int by = (v.enReveal && v.reveal < 12) ? -(12 - v.reveal) : 0;
        if (v.special) {                          // hidden menu : you found the 67
            spr.setTextColor(melange(FOND, teinte(ms / 3 + 180), v.numFade));
            spr.setFreeFont(&Google_Sans_Flex_9pt_SemiBold_150);
            spr.drawString("67", 120 + dx, 100 + dy + by);
            if (v.hehe) {
                spr.setFreeFont(&FreeSansBold9pt7b);
                spr.setTextColor(melange(FOND, OR, v.numFade));
                spr.drawString("hehe you found it", 120, 178);
            }
        } else {                                  // normal number + category
            spr.setTextColor(melange(FOND, TEXTE, v.numFade));
            spr.setFreeFont(&Google_Sans_Flex_9pt_SemiBold_150);
            spr.drawString(texte, 120 + dx, 100 + dy + by);
            spr.setFreeFont(&FreeSansBold12pt7b);
            spr.setTextColor(melange(FOND, GRIS, v.numFade));
            spr.drawString(chance, 120, 182);
        }
    }

    // ----- idle : just the "D" icon at top, and the ADVICE big (main element).
    //        The 1st ("Spin to roll") stays longer. -----
    if (v.attractFade > 0) {
        drawAttractIcon(120, 56, v.attractFade);         // the "D" alone, smaller, at top
        int age = v.attractAge, idx, t, per;
        if (age < FIRST_MS) { idx = 0;                            per = FIRST_MS; t = age; }
        else { int k = (age - FIRST_MS) / 5000; idx = (k + 1) % NB_CONSEILS; per = 5000; t = (age - FIRST_MS) % 5000; }
        int br = 100;
        if (t < 700)            br = t * 100 / 700;
        else if (t > per - 700) br = (per - t) * 100 / 700;
        spr.setFreeFont(&FreeSansBold12pt7b);            // advice = main element
        texteCentre(CONSEILS[idx], 120, 130, melange(FOND, TEXTE, br * v.attractFade / 100));
    }

    spr.pushSprite(0, 0);
}
