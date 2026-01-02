// guy_fawkes.h - Masque Guy Fawkes animé pour écran de démarrage
#ifndef GUY_FAWKES_H
#define GUY_FAWKES_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

class GuyFawkesMask {
public:
    GuyFawkesMask();

    // Dessiner le masque complet (statique)
    void draw(Arduino_GFX* gfx, int centerX, int centerY, int size, uint16_t color);

    // Animation de révélation progressive
    void animateReveal(Arduino_GFX* gfx, int centerX, int centerY, int size,
                       uint16_t maskColor, uint16_t bgColor, int durationMs = 2000);

    // Animation des yeux qui s'allument
    void animateEyes(Arduino_GFX* gfx, int centerX, int centerY, int size,
                     uint16_t eyeColor, int blinkCount = 3);

    // Animation complète de démarrage
    void playStartupAnimation(Arduino_GFX* gfx, int screenWidth, int screenHeight);

    // Version mini pour icône (remplace le logo Bitcoin)
    void drawMini(Arduino_GFX* gfx, int cx, int cy, int size);

    // Animation de pulsation des yeux
    void animateEyesPulse(Arduino_GFX* gfx, int centerX, int centerY, int size, int pulseCount);

    // Animation yeux tres brillants
    void animateEyesBright(Arduino_GFX* gfx, int centerX, int centerY, int size, int blinkCount);

    // Dessiner yeux avec gros halo visible
    void drawGlowingEyes(Arduino_GFX* gfx, int centerX, int centerY, int size);

    // Clignement aleatoire (pour ecran principal)
    void randomBlink(Arduino_GFX* gfx, int cx, int cy, int size);

private:
    // Dessiner le masque flat-top (forme de base)
    void drawMaskFlatTop(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);

    // Animation feu le long des bordures
    void animateFireBorder(Arduino_GFX* gfx, int cx, int cy, int size, int frame);
    // Dessiner les différentes parties
    void drawFaceOutline(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawEyebrows(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawEyes(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawNose(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawMustache(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawSmile(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawGoatee(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);
    void drawCheeks(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color);

    // Utilitaires
    void drawThickLine(Arduino_GFX* gfx, int x0, int y0, int x1, int y1,
                       uint16_t color, int thickness);
    void drawThickArc(Arduino_GFX* gfx, int cx, int cy, int r,
                      int startAngle, int endAngle, uint16_t color, int thickness);
};

extern GuyFawkesMask guyFawkes;

#endif
