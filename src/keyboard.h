// keyboard.h - Clavier virtuel tactile pour SATOSHI AGENT AI ESP32-S3
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Arduino.h>
#include "display.h"
#include "touch.h"

// Dimensions du clavier (ecran paysage 320x240)
#define KB_START_Y      45   // Debut du clavier (apres zone saisie)
#define KB_KEY_WIDTH    28   // Largeur touche
#define KB_KEY_HEIGHT   32   // Hauteur touche
#define KB_KEY_SPACING  3    // Espacement entre touches
#define KB_MAX_INPUT    64   // Longueur max du texte

class VirtualKeyboard {
public:
    VirtualKeyboard();

    // Afficher/masquer le clavier
    void show();
    void hide();
    bool isVisible() { return visible; }

    // Gestion du texte
    void clear();
    const char* getText() { return inputBuffer; }
    int getTextLength() { return inputLength; }

    // Traitement du toucher - retourne: 0=rien, 1=ENTREE, -1=RETOUR
    int handleTouch(int16_t x, int16_t y);

    // Dessiner le clavier
    void draw();

    // Zone de texte en haut
    void drawInputField();

private:
    bool visible;
    char inputBuffer[KB_MAX_INPUT + 1];
    int inputLength;
    bool shiftActive;
    bool symbolMode;

    // Layouts du clavier
    void drawQWERTY();
    void drawSymbols();
    char getKeyAtPosition(int16_t x, int16_t y);
    void addChar(char c);
    void deleteChar();
};

extern VirtualKeyboard keyboard;

#endif
