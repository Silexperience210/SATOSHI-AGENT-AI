// keyboard.cpp - Clavier virtuel tactile pour mode PAYSAGE (320x240)
#include "keyboard.h"

VirtualKeyboard keyboard;

// Layout QWERTY francais simplifie (3 rangees + espace/entree)
const char* KB_ROW1 = "azertyuiop";  // 10 touches
const char* KB_ROW2 = "qsdfghjklm";  // 10 touches
const char* KB_ROW3 = "wxcvbn";       // 6 touches + backspace
const char* KB_ROW1_SHIFT = "AZERTYUIOP";
const char* KB_ROW2_SHIFT = "QSDFGHJKLM";
const char* KB_ROW3_SHIFT = "WXCVBN";
const char* KB_SYMBOLS = "0123456789";

VirtualKeyboard::VirtualKeyboard() {
    visible = false;
    inputLength = 0;
    inputBuffer[0] = '\0';
    shiftActive = false;
    symbolMode = false;
}

void VirtualKeyboard::show() {
    visible = true;
    draw();
}

void VirtualKeyboard::hide() {
    visible = false;
}

void VirtualKeyboard::clear() {
    inputLength = 0;
    inputBuffer[0] = '\0';
    if (visible) {
        drawInputField();
    }
}

void VirtualKeyboard::draw() {
    if (!visible) return;

    Arduino_GFX* gfx = display.getGfx();

    // Effacer tout l'ecran
    gfx->fillScreen(COLOR_BLACK);

    // Bouton RETOUR en haut a gauche
    gfx->fillRoundRect(5, 5, 60, 30, 5, COLOR_RED);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(12, 13);
    gfx->print("RETOUR");

    // Zone de saisie en haut
    drawInputField();

    // Fond du clavier (mode paysage: commence plus haut)
    gfx->fillRect(0, KB_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT - KB_START_Y, COLOR_DARK_GRAY);

    // Dessiner les touches
    if (symbolMode) {
        drawSymbols();
    } else {
        drawQWERTY();
    }
}

void VirtualKeyboard::drawInputField() {
    Arduino_GFX* gfx = display.getGfx();

    // Zone de texte saisi (fond blanc avec bordure) - mode paysage
    gfx->fillRect(70, 5, SCREEN_WIDTH - 80, 35, COLOR_WHITE);
    gfx->drawRect(70, 5, SCREEN_WIDTH - 80, 35, COLOR_ORANGE);

    // Texte saisi
    gfx->setTextColor(COLOR_BLACK);
    gfx->setTextSize(2);

    // Afficher les 20 derniers caracteres si trop long (plus large en paysage)
    int startIdx = 0;
    if (inputLength > 20) {
        startIdx = inputLength - 20;
    }
    gfx->setCursor(75, 12);
    gfx->print(&inputBuffer[startIdx]);

    // Curseur clignotant
    int cursorX = 75 + (inputLength - startIdx) * 12;
    if (cursorX < SCREEN_WIDTH - 15) {
        gfx->fillRect(cursorX, 12, 2, 20, COLOR_ORANGE);
    }
}

void VirtualKeyboard::drawQWERTY() {
    Arduino_GFX* gfx = display.getGfx();

    const char* row1 = shiftActive ? KB_ROW1_SHIFT : KB_ROW1;
    const char* row2 = shiftActive ? KB_ROW2_SHIFT : KB_ROW2;
    const char* row3 = shiftActive ? KB_ROW3_SHIFT : KB_ROW3;

    int y = KB_START_Y + 5;
    int keyW = 28;  // Plus large en paysage
    int keyH = 32;
    int spacing = 3;

    // Rangee 1: AZERTYUIOP (10 touches)
    int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
    for (int i = 0; i < 10; i++) {
        int x = startX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(row1[i]);
    }

    // Rangee 2: QSDFGHJKLM (10 touches)
    y += keyH + spacing;
    for (int i = 0; i < 10; i++) {
        int x = startX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(row2[i]);
    }

    // Rangee 3: Shift + WXCVBN + Backspace
    y += keyH + spacing;
    int row3StartX = 10;

    // Touche Shift
    uint16_t shiftColor = shiftActive ? COLOR_ORANGE : COLOR_LIGHT_GRAY;
    gfx->fillRoundRect(row3StartX, y, 45, keyH, 3, shiftColor);
    gfx->setTextColor(COLOR_BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(row3StartX + 8, y + 12);
    gfx->print("SHIFT");

    // Lettres WXCVBN
    int lettersStartX = row3StartX + 50;
    for (int i = 0; i < 6; i++) {
        int x = lettersStartX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(row3[i]);
    }

    // Touche Backspace
    int bkspX = lettersStartX + 6 * (keyW + spacing) + 10;
    gfx->fillRoundRect(bkspX, y, 50, keyH, 3, COLOR_RED);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(bkspX + 12, y + 8);
    gfx->print("<-");

    // Rangee 4: 123 + Espace + OK
    y += keyH + spacing;

    // Touche 123 (symboles)
    gfx->fillRoundRect(10, y, 50, keyH, 3, COLOR_LIGHT_GRAY);
    gfx->setTextColor(COLOR_BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(18, y + 8);
    gfx->print("123");

    // Barre espace
    gfx->fillRoundRect(65, y, 180, keyH, 3, COLOR_WHITE);
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setTextSize(1);
    gfx->setCursor(130, y + 12);
    gfx->print("ESPACE");

    // Touche OK (Entree)
    gfx->fillRoundRect(250, y, 60, keyH, 3, COLOR_GREEN);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(265, y + 8);
    gfx->print("OK");
}

void VirtualKeyboard::drawSymbols() {
    Arduino_GFX* gfx = display.getGfx();

    int y = KB_START_Y + 5;
    int keyW = 28;
    int keyH = 32;
    int spacing = 3;

    // Rangee 1: 0-9
    int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
    for (int i = 0; i < 10; i++) {
        int x = startX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(KB_SYMBOLS[i]);
    }

    // Rangee 2: Symboles courants
    y += keyH + spacing;
    const char* symbols2 = ".,?!'\"()-+";
    for (int i = 0; i < 10; i++) {
        int x = startX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(symbols2[i]);
    }

    // Rangee 3: Plus de symboles + Backspace
    y += keyH + spacing;
    const char* symbols3 = "@#$%&*:";
    int lettersStartX = 20;
    for (int i = 0; i < 7; i++) {
        int x = lettersStartX + i * (keyW + spacing);
        gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_WHITE);
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 8, y + 8);
        gfx->print(symbols3[i]);
    }

    // Backspace
    int bkspX = lettersStartX + 7 * (keyW + spacing) + 10;
    gfx->fillRoundRect(bkspX, y, 50, keyH, 3, COLOR_RED);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(bkspX + 12, y + 8);
    gfx->print("<-");

    // Rangee 4: ABC + Espace + OK
    y += keyH + spacing;

    // Touche ABC (retour lettres)
    gfx->fillRoundRect(10, y, 50, keyH, 3, COLOR_LIGHT_GRAY);
    gfx->setTextColor(COLOR_BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(18, y + 8);
    gfx->print("ABC");

    // Barre espace
    gfx->fillRoundRect(65, y, 180, keyH, 3, COLOR_WHITE);
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setTextSize(1);
    gfx->setCursor(130, y + 12);
    gfx->print("ESPACE");

    // Touche OK
    gfx->fillRoundRect(250, y, 60, keyH, 3, COLOR_GREEN);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(265, y + 8);
    gfx->print("OK");
}

int VirtualKeyboard::handleTouch(int16_t x, int16_t y) {
    if (!visible) return 0;

    // Bouton RETOUR (en haut a gauche)
    if (x >= 5 && x <= 65 && y >= 5 && y <= 35) {
        return -1;  // Retour
    }

    if (y < KB_START_Y) return 0;

    int keyH = 32;
    int spacing = 3;
    int keyW = 28;

    // Determiner la rangee
    int row = (y - KB_START_Y - 5) / (keyH + spacing);

    if (symbolMode) {
        // Mode symboles
        if (row == 0) {
            // Rangee 1: 0-9
            int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
            int idx = (x - startX) / (keyW + spacing);
            if (idx >= 0 && idx < 10) {
                addChar(KB_SYMBOLS[idx]);
                drawInputField();
            }
        } else if (row == 1) {
            // Rangee 2: symboles
            const char* symbols2 = ".,?!'\"()-+";
            int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
            int idx = (x - startX) / (keyW + spacing);
            if (idx >= 0 && idx < 10) {
                addChar(symbols2[idx]);
                drawInputField();
            }
        } else if (row == 2) {
            // Rangee 3: symboles + backspace
            const char* symbols3 = "@#$%&*:";
            int lettersStartX = 20;
            if (x >= lettersStartX && x < lettersStartX + 7 * (keyW + spacing)) {
                int idx = (x - lettersStartX) / (keyW + spacing);
                if (idx >= 0 && idx < 7) {
                    addChar(symbols3[idx]);
                    drawInputField();
                }
            } else if (x >= 250) {
                // Backspace
                deleteChar();
                drawInputField();
            }
        } else if (row == 3) {
            // Rangee 4: ABC, Espace, OK
            if (x < 65) {
                // ABC - retour lettres
                symbolMode = false;
                draw();
            } else if (x < 250) {
                // Espace
                addChar(' ');
                drawInputField();
            } else {
                // Entree - retourne 1
                return 1;
            }
        }
    } else {
        // Mode QWERTY
        const char* row1 = shiftActive ? KB_ROW1_SHIFT : KB_ROW1;
        const char* row2 = shiftActive ? KB_ROW2_SHIFT : KB_ROW2;
        const char* row3 = shiftActive ? KB_ROW3_SHIFT : KB_ROW3;

        if (row == 0) {
            // Rangee 1: AZERTYUIOP
            int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
            int idx = (x - startX) / (keyW + spacing);
            if (idx >= 0 && idx < 10) {
                addChar(row1[idx]);
                if (shiftActive) {
                    shiftActive = false;
                    draw();
                } else {
                    drawInputField();
                }
            }
        } else if (row == 1) {
            // Rangee 2: QSDFGHJKLM
            int startX = (SCREEN_WIDTH - (10 * keyW + 9 * spacing)) / 2;
            int idx = (x - startX) / (keyW + spacing);
            if (idx >= 0 && idx < 10) {
                addChar(row2[idx]);
                if (shiftActive) {
                    shiftActive = false;
                    draw();
                } else {
                    drawInputField();
                }
            }
        } else if (row == 2) {
            // Rangee 3: Shift + WXCVBN + Backspace
            if (x < 55) {
                // Shift
                shiftActive = !shiftActive;
                draw();
            } else if (x >= 60 && x < 60 + 6 * (keyW + spacing)) {
                // Lettres
                int idx = (x - 60) / (keyW + spacing);
                if (idx >= 0 && idx < 6) {
                    addChar(row3[idx]);
                    if (shiftActive) {
                        shiftActive = false;
                        draw();
                    } else {
                        drawInputField();
                    }
                }
            } else if (x >= 260) {
                // Backspace
                deleteChar();
                drawInputField();
            }
        } else if (row == 3) {
            // Rangee 4: 123, Espace, OK
            if (x < 65) {
                // 123 - mode symboles
                symbolMode = true;
                draw();
            } else if (x < 250) {
                // Espace
                addChar(' ');
                drawInputField();
            } else {
                // Entree - retourne 1
                return 1;
            }
        }
    }

    return 0;
}

void VirtualKeyboard::addChar(char c) {
    if (inputLength < KB_MAX_INPUT) {
        inputBuffer[inputLength++] = c;
        inputBuffer[inputLength] = '\0';
    }
}

void VirtualKeyboard::deleteChar() {
    if (inputLength > 0) {
        inputLength--;
        inputBuffer[inputLength] = '\0';
    }
}
