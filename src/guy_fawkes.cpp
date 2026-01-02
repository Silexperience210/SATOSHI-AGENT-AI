// guy_fawkes.cpp - Implementation du masque Guy Fawkes anime (version amelioree v3)
#include "guy_fawkes.h"
#include <math.h>

GuyFawkesMask guyFawkes;

// Couleurs
#define GF_WHITE      0xFFFF
#define GF_BLACK      0x0000
#define GF_CREAM      0xFFD8  // Couleur creme/ivoire du masque
#define GF_DARK_RED   0xA000  // Rouge fonce pour les joues
#define GF_PINK       0xFBB0  // Rose pour les joues
#define GF_ORANGE     0xFD20  // Bitcoin orange
#define GF_DARK_CREAM 0xEED0  // Ombre pour relief
#define GF_LIGHT_CREAM 0xFFFF // Highlight

GuyFawkesMask::GuyFawkesMask() {
}

void GuyFawkesMask::drawThickLine(Arduino_GFX* gfx, int x0, int y0, int x1, int y1,
                                   uint16_t color, int thickness) {
    for (int i = -thickness/2; i <= thickness/2; i++) {
        for (int j = -thickness/2; j <= thickness/2; j++) {
            gfx->drawLine(x0+i, y0+j, x1+i, y1+j, color);
        }
    }
}

void GuyFawkesMask::drawFaceOutline(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Le masque Guy Fawkes - forme ovale avec front haut et menton pointu
    int w = size * 0.62;   // Largeur au niveau des pommettes
    int h = size * 0.52;   // Hauteur du haut du visage

    // Remplir le visage en creme
    // Haut du visage - ovale
    gfx->fillEllipse(cx, cy - size*0.08, w, h, color);

    // Front plus haut et arrondi
    gfx->fillEllipse(cx, cy - size*0.25, w * 0.85, size * 0.35, color);

    // Partie basse - triangle vers le menton
    int chinY = cy + size * 0.62;
    for (int y = cy + h*0.25; y <= chinY; y++) {
        float progress = (float)(y - (cy + h*0.25)) / (float)(chinY - (cy + h*0.25));
        int halfWidth = w * (1.0 - progress * 0.82);
        gfx->drawFastHLine(cx - halfWidth, y, halfWidth * 2, color);
    }

    // Ombre subtile sur les cotes pour donner du relief
    for (int y = cy - size*0.2; y < cy + size*0.3; y++) {
        gfx->drawPixel(cx - w + 2, y, GF_DARK_CREAM);
        gfx->drawPixel(cx + w - 2, y, GF_DARK_CREAM);
    }

    // Contour du visage en noir
    // Arc superieur (front) - plus prononce
    for (int t = 0; t < 3; t++) {
        gfx->drawEllipse(cx, cy - size*0.08 + t, w, h, GF_BLACK);
    }

    // Arc du front
    for (int t = 0; t < 2; t++) {
        gfx->drawArc(cx, cy - size*0.25, w * 0.85, w * 0.85 - t, 200, 340, GF_BLACK);
    }

    // Lignes laterales vers le menton - plus fluides
    drawThickLine(gfx, cx - w, cy + h*0.25, cx - size*0.10, chinY, GF_BLACK, 2);
    drawThickLine(gfx, cx + w, cy + h*0.25, cx + size*0.10, chinY, GF_BLACK, 2);

    // Pointe du menton plus marquee
    drawThickLine(gfx, cx - size*0.10, chinY, cx, chinY + size*0.10, GF_BLACK, 2);
    drawThickLine(gfx, cx + size*0.10, chinY, cx, chinY + size*0.10, GF_BLACK, 2);
}

void GuyFawkesMask::drawEyebrows(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Sourcils tres arques - signature du masque Guy Fawkes
    // Forme en accent circonflexe inversé, epais et expressifs
    int eyeY = cy - size * 0.10;

    // Sourcil gauche - 3 segments pour une courbe plus naturelle
    for (int t = 0; t < 5; t++) {
        // Partie externe (descend depuis le bord)
        gfx->drawLine(cx - size*0.48, eyeY + size*0.06 + t,
                      cx - size*0.32, eyeY - size*0.06 + t, color);
        // Partie centrale (point haut)
        gfx->drawLine(cx - size*0.32, eyeY - size*0.06 + t,
                      cx - size*0.18, eyeY - size*0.10 + t, color);
        // Partie interne (redescend vers le centre)
        gfx->drawLine(cx - size*0.18, eyeY - size*0.10 + t,
                      cx - size*0.06, eyeY - size*0.04 + t, color);
    }

    // Sourcil droit - miroir exact
    for (int t = 0; t < 5; t++) {
        gfx->drawLine(cx + size*0.48, eyeY + size*0.06 + t,
                      cx + size*0.32, eyeY - size*0.06 + t, color);
        gfx->drawLine(cx + size*0.32, eyeY - size*0.06 + t,
                      cx + size*0.18, eyeY - size*0.10 + t, color);
        gfx->drawLine(cx + size*0.18, eyeY - size*0.10 + t,
                      cx + size*0.06, eyeY - size*0.04 + t, color);
    }

    // Petite ligne verticale entre les sourcils (froncement)
    gfx->drawFastVLine(cx, eyeY - size*0.02, size*0.04, color);
}

void GuyFawkesMask::drawEyes(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Yeux en forme d'amande allongee inclinee
    int eyeY = cy + size * 0.03;
    int eyeSpacing = size * 0.22;
    int eyeW = size * 0.13;
    int eyeH = size * 0.065;

    // Oeil gauche - forme d'amande avec inclinaison
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW, eyeH, color);
    // Double contour pour plus de definition
    gfx->drawEllipse(cx - eyeSpacing, eyeY, eyeW, eyeH, GF_BLACK);
    gfx->drawEllipse(cx - eyeSpacing, eyeY, eyeW+1, eyeH, GF_BLACK);
    // Coins pointus de l'amande
    gfx->drawLine(cx - eyeSpacing - eyeW - 2, eyeY, cx - eyeSpacing - eyeW, eyeY, GF_BLACK);
    gfx->drawLine(cx - eyeSpacing + eyeW, eyeY, cx - eyeSpacing + eyeW + 2, eyeY, GF_BLACK);

    // Oeil droit - miroir
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW, eyeH, color);
    gfx->drawEllipse(cx + eyeSpacing, eyeY, eyeW, eyeH, GF_BLACK);
    gfx->drawEllipse(cx + eyeSpacing, eyeY, eyeW+1, eyeH, GF_BLACK);
    gfx->drawLine(cx + eyeSpacing - eyeW - 2, eyeY, cx + eyeSpacing - eyeW, eyeY, GF_BLACK);
    gfx->drawLine(cx + eyeSpacing + eyeW, eyeY, cx + eyeSpacing + eyeW + 2, eyeY, GF_BLACK);
}

void GuyFawkesMask::drawNose(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Nez fin et droit avec base evasee
    int noseTop = cy + size * 0.10;
    int noseBottom = cy + size * 0.26;

    // Arrete du nez (ligne centrale fine)
    for (int t = 0; t < 2; t++) {
        gfx->drawLine(cx + t, noseTop, cx + t, noseBottom - size*0.04, color);
    }

    // Base du nez - forme evasee
    // Aile gauche
    gfx->drawLine(cx, noseBottom - size*0.04, cx - size*0.05, noseBottom, color);
    gfx->drawLine(cx - size*0.05, noseBottom, cx - size*0.07, noseBottom - size*0.01, color);
    gfx->drawLine(cx - size*0.07, noseBottom - size*0.01, cx - size*0.08, noseBottom - size*0.03, color);

    // Aile droite
    gfx->drawLine(cx, noseBottom - size*0.04, cx + size*0.05, noseBottom, color);
    gfx->drawLine(cx + size*0.05, noseBottom, cx + size*0.07, noseBottom - size*0.01, color);
    gfx->drawLine(cx + size*0.07, noseBottom - size*0.01, cx + size*0.08, noseBottom - size*0.03, color);

    // Narines (petits arcs)
    gfx->fillCircle(cx - size*0.035, noseBottom + size*0.005, 2, color);
    gfx->fillCircle(cx + size*0.035, noseBottom + size*0.005, 2, color);
}

void GuyFawkesMask::drawMustache(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Moustache elegante style mousquetaire - fine et retroussee
    int mustY = cy + size * 0.33;

    // Centre de la moustache (sous le nez)
    for (int t = 0; t < 2; t++) {
        gfx->drawFastHLine(cx - size*0.02, mustY + t, size*0.04, color);
    }

    // Partie gauche - courbe elegante en S qui remonte
    for (int t = 0; t < 3; t++) {
        // Segment 1: horizontal depuis le centre
        gfx->drawLine(cx - size*0.02, mustY + t,
                      cx - size*0.12, mustY - size*0.01 + t, color);
        // Segment 2: courbe vers le bas puis remonte
        gfx->drawLine(cx - size*0.12, mustY - size*0.01 + t,
                      cx - size*0.22, mustY - size*0.06 + t, color);
        // Segment 3: remontee finale elegante
        gfx->drawLine(cx - size*0.22, mustY - size*0.06 + t,
                      cx - size*0.32, mustY - size*0.14 + t, color);
        // Pointe finale
        gfx->drawLine(cx - size*0.32, mustY - size*0.14 + t,
                      cx - size*0.36, mustY - size*0.18 + t, color);
    }

    // Partie droite - miroir parfait
    for (int t = 0; t < 3; t++) {
        gfx->drawLine(cx + size*0.02, mustY + t,
                      cx + size*0.12, mustY - size*0.01 + t, color);
        gfx->drawLine(cx + size*0.12, mustY - size*0.01 + t,
                      cx + size*0.22, mustY - size*0.06 + t, color);
        gfx->drawLine(cx + size*0.22, mustY - size*0.06 + t,
                      cx + size*0.32, mustY - size*0.14 + t, color);
        gfx->drawLine(cx + size*0.32, mustY - size*0.14 + t,
                      cx + size*0.36, mustY - size*0.18 + t, color);
    }
}

void GuyFawkesMask::drawSmile(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Sourire enigmatique - fin et mysterieux
    int smileY = cy + size * 0.43;
    int smileW = size * 0.20;

    // Arc du sourire (courbe douce)
    for (int t = 0; t < 2; t++) {
        for (int i = -smileW; i <= smileW; i++) {
            float x = (float)i / smileW;
            int y = smileY + (int)(x * x * size * 0.05) + t;
            gfx->drawPixel(cx + i, y, color);
        }
    }

    // Coins du sourire retrousses (signature du masque)
    for (int t = 0; t < 2; t++) {
        // Coin gauche
        gfx->drawLine(cx - smileW, smileY + size*0.012 + t,
                      cx - smileW - size*0.03, smileY - size*0.015 + t, color);
        gfx->drawLine(cx - smileW - size*0.03, smileY - size*0.015 + t,
                      cx - smileW - size*0.05, smileY - size*0.03 + t, color);
        // Coin droit
        gfx->drawLine(cx + smileW, smileY + size*0.012 + t,
                      cx + smileW + size*0.03, smileY - size*0.015 + t, color);
        gfx->drawLine(cx + smileW + size*0.03, smileY - size*0.015 + t,
                      cx + smileW + size*0.05, smileY - size*0.03 + t, color);
    }
}

void GuyFawkesMask::drawGoatee(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Barbichette pointue - triangle fin
    int goateeTop = cy + size * 0.48;
    int goateeBottom = cy + size * 0.60;
    int goateeWidth = size * 0.07;

    // Remplissage triangulaire
    for (int y = goateeTop; y <= goateeBottom; y++) {
        float progress = (float)(y - goateeTop) / (float)(goateeBottom - goateeTop);
        int halfWidth = goateeWidth * (1.0 - progress);
        if (halfWidth > 0) {
            gfx->drawFastHLine(cx - halfWidth, y, halfWidth * 2 + 1, color);
        } else {
            gfx->drawPixel(cx, y, color);
        }
    }

    // Contour net
    drawThickLine(gfx, cx - goateeWidth, goateeTop, cx, goateeBottom + 1, color, 1);
    drawThickLine(gfx, cx + goateeWidth, goateeTop, cx, goateeBottom + 1, color, 1);
}

void GuyFawkesMask::drawCheeks(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    // Joues roses caracteristiques - ronds parfaits
    int cheekY = cy + size * 0.14;
    int cheekSpacing = size * 0.30;
    int cheekR = size * 0.085;

    // Joue gauche - degradé du rouge vers rose
    gfx->fillCircle(cx - cheekSpacing, cheekY, cheekR, color);
    gfx->fillCircle(cx - cheekSpacing, cheekY, cheekR - 2, GF_PINK);
    gfx->fillCircle(cx - cheekSpacing - 1, cheekY - 1, cheekR - 4, 0xFCD0); // highlight

    // Joue droite
    gfx->fillCircle(cx + cheekSpacing, cheekY, cheekR, color);
    gfx->fillCircle(cx + cheekSpacing, cheekY, cheekR - 2, GF_PINK);
    gfx->fillCircle(cx + cheekSpacing - 1, cheekY - 1, cheekR - 4, 0xFCD0);
}

void GuyFawkesMask::draw(Arduino_GFX* gfx, int centerX, int centerY, int size, uint16_t color) {
    drawFaceOutline(gfx, centerX, centerY, size, color);
    drawEyebrows(gfx, centerX, centerY, size, GF_BLACK);
    drawEyes(gfx, centerX, centerY, size, GF_BLACK);
    drawNose(gfx, centerX, centerY, size, GF_BLACK);
    drawMustache(gfx, centerX, centerY, size, GF_BLACK);
    drawSmile(gfx, centerX, centerY, size, GF_BLACK);
    drawGoatee(gfx, centerX, centerY, size, GF_BLACK);
}

// Version mini pour icone (sans animation)
void GuyFawkesMask::drawMini(Arduino_GFX* gfx, int cx, int cy, int size) {
    // Masque Guy Fawkes: front PLAT en haut, cotes qui descendent vers menton pointu
    int w = size * 0.42;  // Largeur du masque
    int topY = cy - size * 0.35;  // Haut du masque
    int chinY = cy + size * 0.55;  // Pointe du menton

    // Dessiner le masque avec front plat
    // Partie haute - rectangle avec coins arrondis (front plat)
    int flatTopWidth = w * 0.85;
    gfx->fillRect(cx - flatTopWidth, topY, flatTopWidth * 2, size * 0.15, GF_CREAM);

    // Partie mediane - s'elargit puis se retrecit
    for (int y = topY + size*0.15; y <= cy + size*0.15; y++) {
        float progress = (float)(y - (topY + size*0.15)) / (float)(size * 0.35);
        // S'elargit jusqu'aux pommettes
        int halfW = flatTopWidth + (w - flatTopWidth) * sin(progress * 3.14159 * 0.5);
        gfx->drawFastHLine(cx - halfW, y, halfW * 2, GF_CREAM);
    }

    // Partie basse - retrecit vers le menton
    for (int y = cy + size*0.15; y <= chinY; y++) {
        float progress = (float)(y - (cy + size*0.15)) / (float)(chinY - (cy + size*0.15));
        int halfW = w * (1.0 - progress * 0.92);
        gfx->drawFastHLine(cx - halfW, y, halfW * 2, GF_CREAM);
    }

    // Contour du masque
    // Ligne du front (droite)
    gfx->drawFastHLine(cx - flatTopWidth, topY, flatTopWidth * 2, GF_BLACK);
    // Cotes du haut
    gfx->drawLine(cx - flatTopWidth, topY, cx - w, cy + size*0.15, GF_BLACK);
    gfx->drawLine(cx + flatTopWidth, topY, cx + w, cy + size*0.15, GF_BLACK);
    // Cotes vers le menton
    gfx->drawLine(cx - w, cy + size*0.15, cx - size*0.05, chinY, GF_BLACK);
    gfx->drawLine(cx + w, cy + size*0.15, cx + size*0.05, chinY, GF_BLACK);
    // Pointe du menton
    gfx->drawLine(cx - size*0.05, chinY, cx, chinY + size*0.05, GF_BLACK);
    gfx->drawLine(cx + size*0.05, chinY, cx, chinY + size*0.05, GF_BLACK);

    // Sourcils arques
    gfx->drawLine(cx - size*0.32, cy - size*0.08, cx - size*0.12, cy - size*0.18, GF_BLACK);
    gfx->drawLine(cx - size*0.12, cy - size*0.18, cx - size*0.04, cy - size*0.13, GF_BLACK);
    gfx->drawLine(cx + size*0.32, cy - size*0.08, cx + size*0.12, cy - size*0.18, GF_BLACK);
    gfx->drawLine(cx + size*0.12, cy - size*0.18, cx + size*0.04, cy - size*0.13, GF_BLACK);

    // Yeux en amande
    gfx->fillEllipse(cx - size*0.18, cy - size*0.02, size*0.09, size*0.04, GF_BLACK);
    gfx->fillEllipse(cx + size*0.18, cy - size*0.02, size*0.09, size*0.04, GF_BLACK);

    // Joues roses - position intermediaire (entre 0.28 et 0.38 = 0.33)
    gfx->fillCircle(cx - size*0.33, cy + size*0.08, size*0.055, GF_DARK_RED);
    gfx->fillCircle(cx + size*0.33, cy + size*0.08, size*0.055, GF_DARK_RED);

    // Nez fin
    gfx->drawLine(cx, cy + size*0.05, cx, cy + size*0.18, GF_BLACK);

    // Moustache
    gfx->drawLine(cx - size*0.02, cy + size*0.22, cx - size*0.22, cy + size*0.12, GF_BLACK);
    gfx->drawLine(cx + size*0.02, cy + size*0.22, cx + size*0.22, cy + size*0.12, GF_BLACK);

    // Sourire
    for (int i = -size*0.12; i <= size*0.12; i++) {
        float x = (float)i / (size*0.12);
        int y = cy + size*0.32 + (int)(x * x * size * 0.03);
        gfx->drawPixel(cx + i, y, GF_BLACK);
    }

    // Barbichette
    gfx->fillTriangle(cx - size*0.05, cy + size*0.38,
                      cx + size*0.05, cy + size*0.38,
                      cx, cy + size*0.50, GF_BLACK);
}

// Dessiner le masque style flat-top (meme style que drawMini mais en grand)
void GuyFawkesMask::drawMaskFlatTop(Arduino_GFX* gfx, int cx, int cy, int size, uint16_t color) {
    int w = size * 0.42;
    int topY = cy - size * 0.35;
    int chinY = cy + size * 0.55;
    int flatTopWidth = w * 0.85;

    // Partie haute - front plat
    gfx->fillRect(cx - flatTopWidth, topY, flatTopWidth * 2, size * 0.15, color);

    // Partie mediane
    for (int y = topY + size*0.15; y <= cy + size*0.15; y++) {
        float progress = (float)(y - (topY + size*0.15)) / (float)(size * 0.35);
        int halfW = flatTopWidth + (w - flatTopWidth) * sin(progress * 3.14159 * 0.5);
        gfx->drawFastHLine(cx - halfW, y, halfW * 2, color);
    }

    // Partie basse vers menton
    for (int y = cy + size*0.15; y <= chinY; y++) {
        float progress = (float)(y - (cy + size*0.15)) / (float)(chinY - (cy + size*0.15));
        int halfW = w * (1.0 - progress * 0.92);
        gfx->drawFastHLine(cx - halfW, y, halfW * 2, color);
    }
}

// Animation de feu qui longe les bordures du masque
void GuyFawkesMask::animateFireBorder(Arduino_GFX* gfx, int cx, int cy, int size, int frame) {
    int w = size * 0.42;
    int topY = cy - size * 0.35;
    int chinY = cy + size * 0.55;
    int flatTopWidth = w * 0.85;

    // Couleurs de feu
    uint16_t fireColors[] = {0xF800, 0xFB00, 0xFD20, 0xFFE0, 0xFFFF}; // Rouge -> Orange -> Jaune -> Blanc
    int numColors = 5;

    // Points du contour du masque
    float points[][2] = {
        {(float)(cx - flatTopWidth), (float)topY},           // 0: Haut gauche
        {(float)(cx + flatTopWidth), (float)topY},           // 1: Haut droit
        {(float)(cx + w), (float)(cy + size*0.15)},          // 2: Pommette droite
        {(float)(cx + size*0.05), (float)chinY},             // 3: Menton droit
        {(float)cx, (float)(chinY + size*0.05)},             // 4: Pointe menton
        {(float)(cx - size*0.05), (float)chinY},             // 5: Menton gauche
        {(float)(cx - w), (float)(cy + size*0.15)},          // 6: Pommette gauche
    };
    int numPoints = 7;

    // Position de la particule de feu (se deplace le long du contour)
    int trailLength = 12;
    float totalLength = numPoints * 10.0f; // Approximation
    float pos = fmod(frame * 0.8f, totalLength);

    for (int t = 0; t < trailLength; t++) {
        float trailPos = pos - t * 0.6f;
        if (trailPos < 0) trailPos += totalLength;

        int segment = (int)(trailPos / 10.0f) % numPoints;
        float segProgress = fmod(trailPos, 10.0f) / 10.0f;

        int nextSeg = (segment + 1) % numPoints;
        int px = points[segment][0] + (points[nextSeg][0] - points[segment][0]) * segProgress;
        int py = points[segment][1] + (points[nextSeg][1] - points[segment][1]) * segProgress;

        // Taille et couleur selon position dans la trainee
        int particleSize = max(1, 4 - t / 3);
        int colorIdx = min(t / 3, numColors - 1);

        gfx->fillCircle(px, py, particleSize, fireColors[colorIdx]);

        // Particules d'etincelles aleatoires
        if (t < 3 && random(100) < 40) {
            int sparkX = px + random(-8, 8);
            int sparkY = py + random(-8, 8);
            gfx->drawPixel(sparkX, sparkY, 0xFFE0);
        }
    }
}

void GuyFawkesMask::animateReveal(Arduino_GFX* gfx, int centerX, int centerY, int size,
                                   uint16_t maskColor, uint16_t bgColor, int durationMs) {
    // Nouvelle animation progressive avec style flat-top
    int steps = 15;
    int delayPerStep = durationMs / steps;

    for (int step = 0; step <= steps; step++) {
        gfx->fillScreen(bgColor);

        // Dessiner progressivement le masque flat-top
        if (step >= 1) {
            // Apparition progressive du haut vers le bas
            float reveal = (float)step / steps;
            int revealY = centerY - size*0.35 + (size * 0.9) * reveal;

            // Dessiner seulement la partie revelee
            drawMaskFlatTop(gfx, centerX, centerY, size, maskColor);

            // Masquer ce qui n'est pas encore revele avec fond noir
            if (step < steps) {
                gfx->fillRect(0, revealY, 320, 240 - revealY, bgColor);
            }
        }

        delay(delayPerStep);
    }

    // Masque complet avec details
    drawMini(gfx, centerX, centerY, size);
}

void GuyFawkesMask::animateEyes(Arduino_GFX* gfx, int centerX, int centerY, int size,
                                 uint16_t eyeColor, int blinkCount) {
    int eyeY = centerY - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.09;
    int eyeH = size * 0.04;

    for (int blink = 0; blink < blinkCount; blink++) {
        // Yeux fermes (ligne)
        gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW, eyeH, GF_CREAM);
        gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW, eyeH, GF_CREAM);
        gfx->drawFastHLine(centerX - eyeSpacing - eyeW, eyeY, eyeW*2, GF_BLACK);
        gfx->drawFastHLine(centerX + eyeSpacing - eyeW, eyeY, eyeW*2, GF_BLACK);
        delay(80);

        // Yeux ouverts avec lueur intense
        for (int glow = 0; glow < 3; glow++) {
            // Halo externe
            gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 3 - glow, eyeH + 2 - glow,
                            glow == 0 ? 0x8200 : (glow == 1 ? 0xC300 : eyeColor));
            gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 3 - glow, eyeH + 2 - glow,
                            glow == 0 ? 0x8200 : (glow == 1 ? 0xC300 : eyeColor));
        }
        // Reflet blanc
        gfx->fillCircle(centerX - eyeSpacing - 2, eyeY - 1, 2, GF_WHITE);
        gfx->fillCircle(centerX + eyeSpacing - 2, eyeY - 1, 2, GF_WHITE);
        delay(180);
    }

    // Yeux finaux noirs
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW, eyeH, GF_BLACK);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW, eyeH, GF_BLACK);
}

// Animation de pulsation des yeux orange
void GuyFawkesMask::animateEyesPulse(Arduino_GFX* gfx, int centerX, int centerY, int size, int pulseCount) {
    int eyeY = centerY - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.09;
    int eyeH = size * 0.04;

    for (int pulse = 0; pulse < pulseCount; pulse++) {
        // Pulsation: noir -> orange faible -> orange vif -> orange faible -> noir
        uint16_t pulseColors[] = {GF_BLACK, 0x8200, 0xC300, GF_ORANGE, 0xFD20, GF_ORANGE, 0xC300, 0x8200, GF_BLACK};
        int numPhases = 9;

        for (int phase = 0; phase < numPhases; phase++) {
            // Halo si couleur vive
            if (phase >= 2 && phase <= 6) {
                gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 2, eyeH + 1, 0x4100);
                gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 2, eyeH + 1, 0x4100);
            }

            gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW, eyeH, pulseColors[phase]);
            gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW, eyeH, pulseColors[phase]);

            // Reflet au pic
            if (phase >= 3 && phase <= 5) {
                gfx->fillCircle(centerX - eyeSpacing - 2, eyeY - 1, 1, GF_WHITE);
                gfx->fillCircle(centerX + eyeSpacing - 2, eyeY - 1, 1, GF_WHITE);
            }

            delay(40);
        }
        delay(100);
    }
}

void GuyFawkesMask::playStartupAnimation(Arduino_GFX* gfx, int screenWidth, int screenHeight) {
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2 - 20;
    int maskSize = min(screenWidth, screenHeight) * 0.85;

    // === Phase 1: Fondu inverse - masque apparait du centre vers l'exterieur ===
    gfx->fillScreen(GF_BLACK);

    // Animation de revelation progressive (cercle qui s'agrandit)
    for (int radius = 5; radius <= 180; radius += 6) {
        gfx->fillScreen(GF_BLACK);

        // Dessiner le masque
        drawMini(gfx, centerX, centerY, maskSize);

        // Masquer avec un cercle noir qui retrecit (effet inverse)
        // On dessine des cercles noirs concentriques autour pour cacher progressivement moins
        for (int r = radius + 10; r < 250; r += 2) {
            gfx->drawCircle(centerX, centerY, r, GF_BLACK);
            gfx->drawCircle(centerX, centerY, r + 1, GF_BLACK);
        }

        // Animation de feu pendant la revelation
        if (radius > 40) {
            animateFireBorder(gfx, centerX, centerY, maskSize, radius / 3);
            animateFireBorder(gfx, centerX, centerY, maskSize, 60 - radius / 3);
        }

        delay(25);
    }

    // === Phase 2: Masque complet avec feu final ===
    for (int frame = 0; frame < 30; frame++) {
        gfx->fillScreen(GF_BLACK);
        drawMini(gfx, centerX, centerY, maskSize);
        animateFireBorder(gfx, centerX, centerY, maskSize, frame + 60);
        animateFireBorder(gfx, centerX, centerY, maskSize, 120 - frame);
        delay(30);
    }

    // === Phase 3: Masque stable avec yeux brillants ===
    gfx->fillScreen(GF_BLACK);
    drawMini(gfx, centerX, centerY, maskSize);

    delay(200);

    // Yeux: flash intense
    animateEyesBright(gfx, centerX, centerY, maskSize, 2);

    delay(100);

    // === Phase 4: Texte SATOSHI ===
    gfx->setTextSize(3);
    const char* text = "SATOSHI";
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
    int textX = (screenWidth - tw) / 2;
    int textY = screenHeight - 30;

    // Texte apparait d'un coup
    gfx->setTextColor(GF_ORANGE);
    gfx->setCursor(textX, textY);
    gfx->print(text);

    // === Phase 5: Yeux finaux avec gros halo ===
    delay(200);

    // Dessiner yeux avec halo tres visible
    drawGlowingEyes(gfx, centerX, centerY, maskSize);

    delay(400);
}

// Dessiner les yeux avec un halo tres visible
void GuyFawkesMask::drawGlowingEyes(Arduino_GFX* gfx, int centerX, int centerY, int size) {
    int eyeY = centerY - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.07;  // Reduit de 0.09 a 0.07
    int eyeH = size * 0.035; // Reduit de 0.04 a 0.035

    // Halo externe tres large - rouge sombre
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 8, eyeH + 5, 0x4000);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 8, eyeH + 5, 0x4000);

    // Halo moyen - orange sombre
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 5, eyeH + 3, 0x8200);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 5, eyeH + 3, 0x8200);

    // Halo proche - orange
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 3, eyeH + 2, 0xC300);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 3, eyeH + 2, 0xC300);

    // Oeil orange vif
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 1, eyeH + 1, GF_ORANGE);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 1, eyeH + 1, GF_ORANGE);

    // Coeur jaune
    gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW - 1, eyeH - 1, 0xFFE0);
    gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW - 1, eyeH - 1, 0xFFE0);

    // Point blanc brillant
    gfx->fillCircle(centerX - eyeSpacing - 1, eyeY, 2, GF_WHITE);
    gfx->fillCircle(centerX + eyeSpacing - 1, eyeY, 2, GF_WHITE);
}

// Animation yeux tres brillants avec clignotement
void GuyFawkesMask::animateEyesBright(Arduino_GFX* gfx, int centerX, int centerY, int size, int blinkCount) {
    int eyeY = centerY - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.07;  // Reduit
    int eyeH = size * 0.035;

    for (int blink = 0; blink < blinkCount; blink++) {
        // Yeux fermes - ligne fine
        gfx->fillEllipse(centerX - eyeSpacing, eyeY, eyeW + 3, eyeH + 2, GF_CREAM);
        gfx->fillEllipse(centerX + eyeSpacing, eyeY, eyeW + 3, eyeH + 2, GF_CREAM);
        gfx->drawFastHLine(centerX - eyeSpacing - eyeW - 2, eyeY, eyeW*2 + 4, GF_BLACK);
        gfx->drawFastHLine(centerX + eyeSpacing - eyeW - 2, eyeY, eyeW*2 + 4, GF_BLACK);
        delay(80);

        // Yeux ouverts avec gros halo
        drawGlowingEyes(gfx, centerX, centerY, size);
        delay(250);
    }
}

// Clignement aleatoire des yeux (pour ecran principal)
void GuyFawkesMask::randomBlink(Arduino_GFX* gfx, int cx, int cy, int size) {
    int eyeY = cy - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.07;  // Reduit
    int eyeH = size * 0.035;

    // Fermer les yeux
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW + 2, eyeH + 2, GF_CREAM);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW + 2, eyeH + 2, GF_CREAM);
    gfx->drawFastHLine(cx - eyeSpacing - eyeW, eyeY, eyeW*2, GF_BLACK);
    gfx->drawFastHLine(cx + eyeSpacing - eyeW, eyeY, eyeW*2, GF_BLACK);

    delay(100 + random(50));

    // Rouvrir avec halo
    // Halo externe
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW + 4, eyeH + 3, 0x8200);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW + 4, eyeH + 3, 0x8200);
    // Halo moyen
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW + 2, eyeH + 2, 0xC300);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW + 2, eyeH + 2, 0xC300);
    // Oeil orange
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW, eyeH, GF_ORANGE);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW, eyeH, GF_ORANGE);
    // Coeur jaune
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW - 2, eyeH - 1, 0xFFE0);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW - 2, eyeH - 1, 0xFFE0);
}
