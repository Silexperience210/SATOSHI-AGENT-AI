// display.h - Gestion de l'affichage pour SATOSHI AGENT AI ES3C28P ILI9341V
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

// Couleurs RGB565 standard (avec invertDisplay activé)
#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_ORANGE     0xFD20  // Bitcoin orange RGB565
#define COLOR_GREEN      0x07E0  // Vert pur RGB565
#define COLOR_RED        0xF800  // Rouge RGB565
#define COLOR_BLUE       0x001F  // Bleu RGB565
#define COLOR_DARK_GRAY  0x2104
#define COLOR_LIGHT_GRAY 0x8410

class Display {
public:
    Display();

    bool begin();
    void clear();

    // Ecrans de l'application
    void showWelcome();
    void showSetupMode(const char* ssid, const char* password);
    void showConnecting(const char* ssid);
    void showConnected(const char* ip);
    void showError(const char* message);
    void showMessage(const char* title, const char* message);
    void showSatoshiReady();

    // Interface de conversation
    void showListening();
    void showThinking();
    void showResponse(const char* response);
    void showConversation(const char* userText, const char* satoshiText);

    // Indicateurs audio
    void showRecording();
    void showSpeaking();
    void showVolumeLevel(int level);

    // Lightning/Invoice
    void showInvoice(const char* bolt11, int64_t amountSats);

    // Mining
    void showMiningMenu();
    void showMiningStats();
    void showMinersList();
    void showAddMinerKeyboard();

    // Utilitaires
    void drawCenteredText(const char* text, int y, uint16_t color, int textSize = 2);
    void drawBitcoinLogo(int x, int y, int size);
    void drawWrappedText(const char* text, int x, int y, int maxWidth, uint16_t color, int textSize = 2);
    void drawMicIcon(int x, int y, bool active);
    void drawSpeakerIcon(int x, int y, bool active);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t bgColor, uint16_t textColor);

    Arduino_GFX* getGfx() { return gfx; }

private:
    Arduino_DataBus* bus;
    Arduino_GFX* gfx;
};

extern Display display;

#endif
