// display.cpp - Implementation affichage SATOSHI AGENT AI ES3C28P ILI9341V
#include "display.h"
#include "qrcode.h"
#include "mining_manager.h"
#include "guy_fawkes.h"

Display display;

// Fonction pour convertir les caractères UTF-8 accentués en ASCII
// Gère les caractères français courants (é, è, ê, à, ù, ç, etc.)
static void utf8ToAscii(char* dest, const char* src, size_t maxLen) {
    size_t i = 0, j = 0;
    while (src[i] && j < maxLen - 1) {
        unsigned char c = (unsigned char)src[i];

        if (c < 0x80) {
            // ASCII standard
            dest[j++] = src[i++];
        } else if ((c & 0xE0) == 0xC0) {
            // Caractère UTF-8 sur 2 octets
            unsigned char c2 = (unsigned char)src[i + 1];
            uint16_t code = ((c & 0x1F) << 6) | (c2 & 0x3F);

            // Remplacements français
            switch (code) {
                case 0xE0: dest[j++] = 'a'; break;  // à
                case 0xE2: dest[j++] = 'a'; break;  // â
                case 0xE4: dest[j++] = 'a'; break;  // ä
                case 0xE8: dest[j++] = 'e'; break;  // è
                case 0xE9: dest[j++] = 'e'; break;  // é
                case 0xEA: dest[j++] = 'e'; break;  // ê
                case 0xEB: dest[j++] = 'e'; break;  // ë
                case 0xEC: dest[j++] = 'i'; break;  // ì
                case 0xEE: dest[j++] = 'i'; break;  // î
                case 0xEF: dest[j++] = 'i'; break;  // ï
                case 0xF2: dest[j++] = 'o'; break;  // ò
                case 0xF4: dest[j++] = 'o'; break;  // ô
                case 0xF6: dest[j++] = 'o'; break;  // ö
                case 0xF9: dest[j++] = 'u'; break;  // ù
                case 0xFB: dest[j++] = 'u'; break;  // û
                case 0xFC: dest[j++] = 'u'; break;  // ü
                case 0xE7: dest[j++] = 'c'; break;  // ç
                case 0xC0: dest[j++] = 'A'; break;  // À
                case 0xC2: dest[j++] = 'A'; break;  // Â
                case 0xC8: dest[j++] = 'E'; break;  // È
                case 0xC9: dest[j++] = 'E'; break;  // É
                case 0xCA: dest[j++] = 'E'; break;  // Ê
                case 0xCE: dest[j++] = 'I'; break;  // Î
                case 0xD4: dest[j++] = 'O'; break;  // Ô
                case 0xD9: dest[j++] = 'U'; break;  // Ù
                case 0xDB: dest[j++] = 'U'; break;  // Û
                case 0xC7: dest[j++] = 'C'; break;  // Ç
                case 0xB0: dest[j++] = '*'; break;  // ° (degré)
                default: dest[j++] = '?'; break;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // Caractère UTF-8 sur 3 octets (euro, etc.)
            dest[j++] = '?';
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // Caractère UTF-8 sur 4 octets (emojis)
            dest[j++] = ' ';
            i += 4;
        } else {
            // Octet invalide
            i++;
        }
    }
    dest[j] = '\0';
}

Display::Display() {
    // SPI pour ILI9341V
    bus = new Arduino_ESP32SPI(
        PIN_LCD_DC,     // DC
        PIN_LCD_CS,     // CS
        PIN_LCD_SCLK,   // SCK
        PIN_LCD_MOSI,   // MOSI
        PIN_LCD_MISO,   // MISO
        FSPI            // SPI bus (FSPI for ESP32-S3)
    );

    // ILI9341 driver (240x320)
    gfx = new Arduino_ILI9341(
        bus,
        PIN_LCD_RST,    // RST (-1 = shared with ESP32 reset)
        0,              // Rotation
        false           // IPS mode (false for standard TFT)
    );
}

bool Display::begin() {
    // Configurer le backlight
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);  // Allumer le backlight

    if (!gfx->begin()) {
        Serial.println("Erreur: Échec initialisation ILI9341");
        return false;
    }

    gfx->setRotation(1);  // Paysage (320x240) - rotation 90 degrés
    gfx->invertDisplay(true);  // Inverser les couleurs pour ce panel
    clear();

    Serial.printf("Display ILI9341 initialisé: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    return true;
}

void Display::clear() {
    gfx->fillScreen(COLOR_BLACK);
}

void Display::drawCenteredText(const char* text, int y, uint16_t color, int textSize) {
    // Convertir UTF-8 en ASCII pour affichage
    char asciiText[256];
    utf8ToAscii(asciiText, text, sizeof(asciiText));

    gfx->setTextSize(textSize);
    gfx->setTextColor(color);

    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(asciiText, 0, 0, &x1, &y1, &w, &h);

    int x = (SCREEN_WIDTH - w) / 2;
    gfx->setCursor(x, y);
    gfx->print(asciiText);
}

void Display::drawBitcoinLogo(int x, int y, int size) {
    gfx->fillCircle(x, y, size, COLOR_ORANGE);
    gfx->setTextSize(size / 8);
    gfx->setTextColor(COLOR_WHITE);

    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds("B", 0, 0, &x1, &y1, &w, &h);
    gfx->setCursor(x - w/2, y - h/2);
    gfx->print("B");
}

void Display::drawMicIcon(int x, int y, bool active) {
    uint16_t color = active ? COLOR_RED : COLOR_LIGHT_GRAY;
    // Microphone simple (sans drawArc qui peut crasher)
    gfx->fillRoundRect(x - 6, y - 15, 12, 20, 5, color);
    // Arc simplifié avec lignes
    gfx->drawLine(x - 10, y + 5, x - 10, y + 10, color);
    gfx->drawLine(x + 10, y + 5, x + 10, y + 10, color);
    gfx->drawLine(x - 10, y + 10, x, y + 15, color);
    gfx->drawLine(x + 10, y + 10, x, y + 15, color);
    gfx->drawLine(x, y + 15, x, y + 20, color);
    gfx->drawLine(x - 6, y + 20, x + 6, y + 20, color);
}

void Display::drawSpeakerIcon(int x, int y, bool active) {
    uint16_t color = active ? COLOR_GREEN : COLOR_LIGHT_GRAY;
    // Speaker simple (sans drawArc qui peut crasher)
    gfx->fillRect(x - 8, y - 6, 8, 12, color);
    gfx->fillTriangle(x, y - 12, x, y + 12, x + 12, y, color);
    if (active) {
        // Ondes simplifiées avec lignes
        gfx->drawLine(x + 14, y - 4, x + 18, y - 6, color);
        gfx->drawLine(x + 14, y + 4, x + 18, y + 6, color);
    }
}

void Display::showWelcome() {
    // Animation du masque Guy Fawkes
    guyFawkes.playStartupAnimation(gfx, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Display::showSetupMode(const char* ssid, const char* password) {
    clear();

    drawCenteredText("Configuration", 15, COLOR_ORANGE, 2);

    drawCenteredText("Connectez-vous au WiFi:", 50, COLOR_WHITE, 1);

    char buf[80];
    snprintf(buf, sizeof(buf), "SSID: %s", ssid);
    drawCenteredText(buf, 80, COLOR_GREEN, 2);

    snprintf(buf, sizeof(buf), "Pass: %s", password);
    drawCenteredText(buf, 110, COLOR_GREEN, 2);

    drawCenteredText("Puis ouvrez:", 150, COLOR_WHITE, 1);
    drawCenteredText("http://192.168.4.1", 175, COLOR_ORANGE, 2);
}

void Display::showConnecting(const char* ssid) {
    clear();

    drawBitcoinLogo(SCREEN_WIDTH / 2, 60, 35);

    drawCenteredText("Connexion...", 120, COLOR_WHITE, 2);

    char buf[80];
    snprintf(buf, sizeof(buf), "%s", ssid);
    drawCenteredText(buf, 160, COLOR_ORANGE, 2);
}

void Display::showConnected(const char* ip) {
    clear();

    drawBitcoinLogo(SCREEN_WIDTH / 2, 60, 35);

    drawCenteredText("Connecté!", 120, COLOR_GREEN, 2);

    char buf[80];
    snprintf(buf, sizeof(buf), "IP: %s", ip);
    drawCenteredText(buf, 160, COLOR_WHITE, 2);
}

void Display::showError(const char* message) {
    clear();

    drawCenteredText("Erreur", 80, COLOR_RED, 2);
    drawCenteredText(message, 150, COLOR_WHITE, 2);
}

void Display::showMessage(const char* title, const char* message) {
    clear();

    drawCenteredText(title, 80, COLOR_ORANGE, 2);
    drawCenteredText(message, 160, COLOR_WHITE, 2);
}

void Display::showSatoshiReady() {
    clear();

    // Mode paysage: masque Guy Fawkes à gauche (remplace le logo Bitcoin)
    guyFawkes.drawMini(gfx, 50, 50, 55);

    // Yeux orange brillants sur le masque mini (taille reduite)
    int cx = 50, cy = 50, size = 55;
    int eyeY = cy - size * 0.02;
    int eyeSpacing = size * 0.18;
    int eyeW = size * 0.07;  // Reduit de 0.09 a 0.07
    int eyeH = size * 0.035; // Reduit de 0.04 a 0.035
    // Halo orange
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW + 1, eyeH + 1, 0xC300);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW + 1, eyeH + 1, 0xC300);
    // Coeur orange vif
    gfx->fillEllipse(cx - eyeSpacing, eyeY, eyeW, eyeH, COLOR_ORANGE);
    gfx->fillEllipse(cx + eyeSpacing, eyeY, eyeW, eyeH, COLOR_ORANGE);

    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(100, 30);
    gfx->print("SATOSHI");

    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setCursor(100, 60);
    gfx->print("Ton assistant Bitcoin");

    // Boutons en mode paysage - 4 boutons sur 2 lignes
    // Ligne 1: ECRIRE, MINING
    drawButton(10, 100, 145, 45, "ECRIRE", COLOR_GREEN, COLOR_WHITE);
    drawButton(165, 100, 145, 45, "MINING", COLOR_ORANGE, COLOR_WHITE);

    // Ligne 2: INVOICE 21, PARLER
    drawButton(10, 155, 145, 45, "INVOICE", COLOR_ORANGE, COLOR_WHITE);
    drawButton(165, 155, 145, 45, "PARLER", COLOR_GREEN, COLOR_WHITE);
}

void Display::drawButton(int x, int y, int w, int h, const char* label, uint16_t bgColor, uint16_t textColor) {
    // Rectangle arrondi pour le bouton
    gfx->fillRoundRect(x, y, w, h, 8, bgColor);
    gfx->drawRoundRect(x, y, w, h, 8, COLOR_WHITE);

    // Centrer le texte dans le bouton
    gfx->setTextSize(2);
    gfx->setTextColor(textColor);

    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);

    int textX = x + (w - tw) / 2;
    int textY = y + (h - th) / 2;
    gfx->setCursor(textX, textY);
    gfx->print(label);
}

void Display::showListening() {
    clear();

    // Masque Guy Fawkes au centre
    guyFawkes.drawMini(gfx, SCREEN_WIDTH / 2, 55, 70);
    drawCenteredText("SATOSHI", 110, COLOR_ORANGE, 2);

    // Grand cercle rouge pour l'écoute
    gfx->fillCircle(SCREEN_WIDTH / 2, 200, 40, COLOR_RED);
    drawMicIcon(SCREEN_WIDTH / 2, 200, true);

    drawCenteredText("Écoute...", 280, COLOR_WHITE, 2);
}

void Display::showRecording() {
    clear();

    // Masque Guy Fawkes au centre
    guyFawkes.drawMini(gfx, SCREEN_WIDTH / 2, 50, 60);
    drawCenteredText("SATOSHI", 95, COLOR_ORANGE, 2);

    // Animation enregistrement
    gfx->fillCircle(SCREEN_WIDTH / 2, 190, 45, COLOR_RED);
    gfx->drawCircle(SCREEN_WIDTH / 2, 190, 55, COLOR_RED);
    gfx->drawCircle(SCREEN_WIDTH / 2, 190, 65, COLOR_RED);

    drawCenteredText("Enregistrement...", 280, COLOR_WHITE, 2);
}

void Display::showThinking() {
    clear();

    // Masque Guy Fawkes au centre
    guyFawkes.drawMini(gfx, SCREEN_WIDTH / 2, 55, 70);
    drawCenteredText("SATOSHI", 110, COLOR_ORANGE, 2);

    // Points de réflexion
    drawCenteredText("...", 190, COLOR_ORANGE, 4);
    drawCenteredText("Réflexion...", 260, COLOR_WHITE, 2);
}

void Display::showSpeaking() {
    clear();

    // Masque Guy Fawkes au centre
    guyFawkes.drawMini(gfx, SCREEN_WIDTH / 2, 55, 70);
    drawCenteredText("SATOSHI", 110, COLOR_ORANGE, 2);

    // Cercle vert pour parler
    gfx->fillCircle(SCREEN_WIDTH / 2, 200, 40, COLOR_GREEN);

    drawSpeakerIcon(SCREEN_WIDTH / 2, 200, true);
    drawCenteredText("Parle...", 280, COLOR_WHITE, 2);
}

void Display::showVolumeLevel(int level) {
    // Barre de volume en bas de l'écran
    int barWidth = (SCREEN_WIDTH - 40) * level / 100;
    gfx->fillRect(20, SCREEN_HEIGHT - 25, SCREEN_WIDTH - 40, 15, COLOR_DARK_GRAY);
    gfx->fillRect(20, SCREEN_HEIGHT - 25, barWidth, 15, COLOR_GREEN);
}

void Display::showResponse(const char* response) {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 40, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 12);
    gfx->print("SATOSHI");

    // Zone de réponse
    drawWrappedText(response, 10, 55, SCREEN_WIDTH - 20, COLOR_WHITE, 2);
}

void Display::showConversation(const char* userText, const char* satoshiText) {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 35, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 10);
    gfx->print("SATOSHI");

    // Message utilisateur
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setCursor(10, 45);
    gfx->print("Vous:");

    drawWrappedText(userText, 10, 58, SCREEN_WIDTH - 20, COLOR_BLUE, 1);

    // Ligne de séparation
    gfx->drawLine(10, 110, SCREEN_WIDTH - 10, 110, COLOR_DARK_GRAY);

    // Réponse SATOSHI
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(10, 120);
    gfx->print("SATOSHI:");

    drawWrappedText(satoshiText, 10, 135, SCREEN_WIDTH - 20, COLOR_WHITE, 1);
}

void Display::showInvoice(const char* bolt11, int64_t amountSats) {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 7);
    gfx->print("INVOICE");

    // Montant
    char amountStr[32];
    snprintf(amountStr, sizeof(amountStr), "%lld sats", amountSats);
    gfx->setCursor(SCREEN_WIDTH - 90, 7);
    gfx->print(amountStr);

    // Préparer le texte du QR - BOLT11 en majuscules pour meilleure compression
    String qrText = String(bolt11);
    qrText.toUpperCase();

    // Déterminer la version QR nécessaire selon la longueur
    // Version 10 = 57x57 modules, ~174 alphanumériques avec ECC_LOW
    // Version 15 = 77x77 modules, ~412 alphanumériques avec ECC_LOW
    // Version 20 = 97x97 modules, ~858 alphanumériques avec ECC_LOW
    int qrVersion = 10;
    int textLen = qrText.length();

    if (textLen > 400) {
        qrVersion = 20;
    } else if (textLen > 174) {
        qrVersion = 15;
    }

    Serial.printf("QR: len=%d, version=%d\n", textLen, qrVersion);

    // Allouer le buffer QR avec la bonne taille
    QRCode qrcode;
    int bufferSize = qrcode_getBufferSize(qrVersion);
    uint8_t* qrcodeData = (uint8_t*)malloc(bufferSize);

    if (!qrcodeData) {
        Serial.println("Erreur: malloc QR échoué");
        gfx->setTextColor(COLOR_RED);
        gfx->setCursor(50, 120);
        gfx->print("Erreur mémoire QR");
        return;
    }

    // Générer le QR code
    int qrResult = qrcode_initText(&qrcode, qrcodeData, qrVersion, ECC_LOW, qrText.c_str());

    if (qrResult != 0) {
        Serial.printf("Erreur QR: %d\n", qrResult);
        free(qrcodeData);
        gfx->setTextColor(COLOR_RED);
        gfx->setCursor(50, 120);
        gfx->print("Invoice trop longue");
        return;
    }

    // Calculer la taille du QR code sur l'écran
    int qrModules = qrcode.size;
    int maxQrSize = 160;  // Taille max en pixels
    int moduleSize = maxQrSize / qrModules;
    if (moduleSize < 1) moduleSize = 1;
    int qrPixelSize = qrModules * moduleSize;

    int qrX = (SCREEN_WIDTH - qrPixelSize) / 2;
    int qrY = 35;

    // Fond blanc avec bordure
    int border = 4;
    gfx->fillRect(qrX - border, qrY - border,
                  qrPixelSize + border * 2, qrPixelSize + border * 2, COLOR_WHITE);

    // Dessiner le QR code
    for (int y = 0; y < qrModules; y++) {
        for (int x = 0; x < qrModules; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                gfx->fillRect(qrX + x * moduleSize, qrY + y * moduleSize,
                             moduleSize, moduleSize, COLOR_BLACK);
            }
        }
    }

    // Libérer la mémoire
    free(qrcodeData);

    // Message sous le QR
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    int textY = qrY + qrPixelSize + 8;
    gfx->setCursor((SCREEN_WIDTH - 108) / 2, textY);
    gfx->print("Scannez pour payer");

    // Afficher les premiers caractères du BOLT11
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setCursor(5, SCREEN_HEIGHT - 30);
    char shortBolt[52];
    strncpy(shortBolt, bolt11, 45);
    shortBolt[45] = '\0';
    strcat(shortBolt, "...");
    gfx->print(shortBolt);

    // Instruction
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(5, SCREEN_HEIGHT - 15);
    gfx->print("Touchez pour fermer");
}

void Display::drawWrappedText(const char* text, int x, int y, int maxWidth, uint16_t color, int textSize) {
    if (!text || strlen(text) == 0) return;

    gfx->setTextSize(textSize);
    gfx->setTextColor(color);

    int charWidth = 6 * textSize;
    int lineHeight = 8 * textSize + 4;
    int charsPerLine = maxWidth / charWidth;
    if (charsPerLine < 1) charsPerLine = 1;

    // Convertir UTF-8 en ASCII
    char buffer[512];
    utf8ToAscii(buffer, text, sizeof(buffer));

    int currentY = y;
    int pos = 0;
    int len = strlen(buffer);

    while (pos < len && currentY < SCREEN_HEIGHT - lineHeight) {
        int lineEnd = pos + charsPerLine;
        if (lineEnd >= len) {
            lineEnd = len;
        } else {
            // Chercher le dernier espace
            int lastSpace = -1;
            for (int i = lineEnd; i > pos; i--) {
                if (buffer[i] == ' ') {
                    lastSpace = i;
                    break;
                }
            }
            if (lastSpace > pos) {
                lineEnd = lastSpace;
            }
        }

        // Copier la ligne
        char line[64];
        int lineLen = lineEnd - pos;
        if (lineLen > 63) lineLen = 63;
        strncpy(line, buffer + pos, lineLen);
        line[lineLen] = '\0';

        gfx->setCursor(x, currentY);
        gfx->print(line);

        pos = lineEnd;
        if (pos < len && buffer[pos] == ' ') {
            pos++;
        }
        currentY += lineHeight;
    }
}

// ============================================================
// Écrans Mining
// ============================================================

void Display::showMiningMenu() {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 35, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 10);
    gfx->print("MINING");

    // Afficher nombre de mineurs
    int count = miningManager.getMinerCount();
    char countStr[16];
    snprintf(countStr, sizeof(countStr), "%d mineur%s", count, count > 1 ? "s" : "");
    gfx->setCursor(SCREEN_WIDTH - 100, 10);
    gfx->print(countStr);

    // Boutons du menu mining
    // STATS - voir les statistiques agrégées
    drawButton(10, 50, 145, 40, "STATS", COLOR_GREEN, COLOR_WHITE);

    // LISTE - voir la liste des mineurs
    drawButton(165, 50, 145, 40, "LISTE", COLOR_BLUE, COLOR_WHITE);

    // AJOUTER - ajouter un mineur via clavier
    drawButton(10, 100, 145, 40, "AJOUTER", COLOR_GREEN, COLOR_WHITE);

    // REFRESH - rafraîchir les données
    drawButton(165, 100, 145, 40, "REFRESH", COLOR_BLUE, COLOR_WHITE);

    // RETOUR
    drawButton(10, 150, 300, 40, "RETOUR", COLOR_DARK_GRAY, COLOR_WHITE);

    // Stats rapides en bas
    if (count > 0) {
        MiningStats stats = miningManager.getAggregatedStats();
        gfx->setTextSize(1);
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setCursor(10, SCREEN_HEIGHT - 30);
        char quickStats[64];
        snprintf(quickStats, sizeof(quickStats), "Total: %s | %.1fW | %.1f°C",
                 miningManager.formatTotalHashrate().c_str(),
                 stats.totalPower, stats.avgTemp);
        gfx->print(quickStats);
    }
}

void Display::showMiningStats() {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 7);
    gfx->print("STATS MINING");

    int count = miningManager.getMinerCount();
    if (count == 0) {
        drawCenteredText("Aucun mineur", 80, COLOR_LIGHT_GRAY, 2);
        drawCenteredText("configuré", 110, COLOR_LIGHT_GRAY, 2);
        drawCenteredText("Touchez pour fermer", SCREEN_HEIGHT - 20, COLOR_ORANGE, 1);
        return;
    }

    MiningStats stats = miningManager.getAggregatedStats();

    int y = 40;
    gfx->setTextSize(1);

    // Mineurs en ligne
    gfx->setTextColor(COLOR_GREEN);
    gfx->setCursor(10, y);
    char buf[64];
    snprintf(buf, sizeof(buf), "En ligne: %d/%d", stats.onlineCount, stats.minerCount);
    gfx->print(buf);
    y += 18;

    // Hashrate total
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(10, y);
    snprintf(buf, sizeof(buf), "Hashrate: %s", miningManager.formatTotalHashrate().c_str());
    gfx->print(buf);
    y += 18;

    // Consommation
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, y);
    snprintf(buf, sizeof(buf), "Conso: %.1f W", stats.totalPower);
    gfx->print(buf);
    y += 18;

    // Efficacité
    gfx->setCursor(10, y);
    snprintf(buf, sizeof(buf), "Efficacité: %.1f J/TH", stats.avgEfficiency);
    gfx->print(buf);
    y += 18;

    // Température moyenne
    gfx->setCursor(10, y);
    snprintf(buf, sizeof(buf), "Temp moy: %.1f°C", stats.avgTemp);
    gfx->print(buf);
    y += 18;

    // Shares
    gfx->setTextColor(COLOR_GREEN);
    gfx->setCursor(10, y);
    snprintf(buf, sizeof(buf), "Shares: %u acceptés", stats.totalSharesAccepted);
    gfx->print(buf);
    y += 18;

    if (stats.totalSharesRejected > 0) {
        gfx->setTextColor(COLOR_RED);
        gfx->setCursor(10, y);
        float rejectRate = 100.0f * stats.totalSharesRejected /
                          (stats.totalSharesAccepted + stats.totalSharesRejected);
        snprintf(buf, sizeof(buf), "Rejetés: %u (%.1f%%)", stats.totalSharesRejected, rejectRate);
        gfx->print(buf);
    }

    // Instruction
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(10, SCREEN_HEIGHT - 15);
    gfx->print("Touchez pour fermer");
}

void Display::showMinersList() {
    clear();

    // Header
    gfx->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 7);
    gfx->print("MINEURS");

    int count = miningManager.getMinerCount();
    if (count == 0) {
        drawCenteredText("Aucun mineur", 80, COLOR_LIGHT_GRAY, 2);
        drawCenteredText("Appuyez AJOUTER", 110, COLOR_LIGHT_GRAY, 2);
        drawCenteredText("Touchez pour fermer", SCREEN_HEIGHT - 20, COLOR_ORANGE, 1);
        return;
    }

    int y = 35;
    gfx->setTextSize(1);

    for (int i = 0; i < count && y < SCREEN_HEIGHT - 30; i++) {
        BitaxeMinerInfo* m = miningManager.getMiner(i);
        if (!m) continue;

        // Indicateur de statut
        if (m->isOnline) {
            gfx->fillCircle(15, y + 6, 4, COLOR_GREEN);
        } else {
            gfx->fillCircle(15, y + 6, 4, COLOR_RED);
        }

        // Nom du mineur
        gfx->setTextColor(COLOR_WHITE);
        gfx->setCursor(25, y);
        char line[64];
        if (m->isOnline) {
            snprintf(line, sizeof(line), "%s: %.1f GH/s %.1fW %.0f°C",
                     m->hostname[0] ? m->hostname : m->ip,
                     m->hashrate, m->power, m->tempChip);
        } else {
            snprintf(line, sizeof(line), "%s: HORS LIGNE", m->ip);
        }
        gfx->print(line);

        y += 20;
    }

    // Instruction
    gfx->setTextColor(COLOR_ORANGE);
    gfx->setCursor(10, SCREEN_HEIGHT - 15);
    gfx->print("Touchez pour fermer");
}

void Display::showAddMinerKeyboard() {
    // Cette fonction prépare l'écran pour l'ajout de mineur
    // Le clavier virtuel sera géré par keyboard.cpp
    clear();

    gfx->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_ORANGE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(10, 7);
    gfx->print("AJOUTER MINEUR");

    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_LIGHT_GRAY);
    gfx->setCursor(10, 40);
    gfx->print("Entrez l'adresse IP du Bitaxe:");
    gfx->setCursor(10, 55);
    gfx->print("Exemple: 192.168.1.100");
}
