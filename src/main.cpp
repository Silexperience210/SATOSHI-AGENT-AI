// SATOSHI AGENT AI - Assistant Bitcoin Vocal pour ESP32-S3
// main.cpp - Version avec wake word "SATOSHI"

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display.h"
#include "captive_portal.h"
#include "claude_api.h"
#include "bitcoin_api.h"
#include "audio.h"
#include "tts_groq.h"
#include "tts_google.h"
#include "whisper_api.h"
#include "wake_word.h"
#include "touch.h"
#include "lnbits_api.h"
// es8311.h remplacé par freenove_es8311.h dans audio.cpp
#include "keyboard.h"
#include "rgb_led.h"
#include "mining_manager.h"
#include "guy_fawkes.h"
#include "braiins_api.h"

// États de l'application
enum AppState {
    STATE_INIT,
    STATE_SETUP_PORTAL,
    STATE_CONNECTING,
    STATE_FETCHING_DATA,
    STATE_READY,
    STATE_WAKE_LISTENING,    // Écoute du wake word
    STATE_COMMAND_LISTENING, // Enregistrement de la commande
    STATE_TRANSCRIBING,
    STATE_PROCESSING,
    STATE_SPEAKING,
    STATE_ERROR
};

AppState currentState = STATE_INIT;
unsigned long stateTimer = 0;
unsigned long lastDataRefresh = 0;
unsigned long lastLevelUpdate = 0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkDelay = 0;  // Délai avant prochain clignement
int wifiRetryCount = 0;
const int MAX_WIFI_RETRIES = 20;
const unsigned long DATA_REFRESH_INTERVAL = 60000;

// Pin du bouton (backup si wake word ne fonctionne pas)
#define BUTTON_PIN 0  // Boot button sur ESP32-S3

String lastUserMessage = "";
String lastResponse = "";
bool buttonPressed = false;
bool useWakeWord = true;   // Activer wake word "SATOSHI"
String serialBuffer = "";  // Buffer pour commandes serie

void connectToWiFi();
void processSerialCommand();
void processVoiceCommand();
void askClaude(const String& question);
void refreshBitcoinData();
String buildBitcoinContext();
void handleMiningMenu();
bool inMiningMenu = false;

// Fonction TTS - Groq Orpheus (3600 tokens/jour)
bool speakText(const char* text, uint8_t** outBuffer, size_t* outSize) {
    Serial.println("TTS: Utilisation Groq Orpheus");
    resetTTSRateLimitInfo();  // Reset avant chaque appel
    return getTTSWavBuffer(text, outBuffer, outSize);
}

// Délai interruptible par touch - retourne true si interrompu
bool delayWithTouchCheck(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (touch.touched()) {
            Serial.println("Touch détecté - interruption!");
            delay(150);  // Debounce
            return true;  // Interrompu
        }
        delay(20);
    }
    return false;  // Pas interrompu
}

// Variable globale pour signaler interruption
volatile bool dialogueInterrupted = false;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n========================================");
    Serial.println("   SATOSHI AGENT AI - Assistant Bitcoin Vocal");
    Serial.println("     Version SATOSHI AGENT AI ESP32-S3 2.8\"");
    Serial.println("========================================\n");

    // Bouton pour activer l'écoute (backup)
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialiser l'écran
    Serial.println("Initialisation de l'écran...");
    if (!display.begin()) {
        Serial.println("ERREUR: Échec initialisation écran!");
        while (1) delay(1000);
    }
    Serial.printf("Écran OK - %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);

    // Afficher l'écran de bienvenue
    display.showWelcome();
    delay(1000);

    // Charger la configuration
    Serial.println("Chargement de la configuration...");
    configManager.begin();

    // Vérifier si configuration nécessaire
    if (!configManager.isConfigured()) {
        Serial.println("Première utilisation - démarrage du portail de configuration");
        currentState = STATE_SETUP_PORTAL;
        captivePortal.begin();
    } else {
        Serial.println("Configuration trouvée - connexion au WiFi");
        currentState = STATE_CONNECTING;
        connectToWiFi();
    }
}

void connectToWiFi() {
    display.showConnecting(configManager.config.wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(configManager.config.wifi_ssid, configManager.config.wifi_password);

    Serial.printf("Connexion a %s", configManager.config.wifi_ssid);
    wifiRetryCount = 0;
    stateTimer = millis();
}

void refreshBitcoinData() {
    Serial.println("\n--- Rafraîchissement données Bitcoin ---");

    display.showMessage("SATOSHI", "Mise à jour...");

    bitcoinAPI.fetchPrice();
    bitcoinAPI.fetchFees();
    bitcoinAPI.fetchBlockHeight();
    bitcoinAPI.fetchMempoolInfo();
    bitcoinAPI.fetchHashRate();
    bitcoinAPI.fetchLightningStats();
    lnbitsAPI.fetchBalance();

    String context = buildBitcoinContext();
    claudeAPI.updateBitcoinContext(context);

    lastDataRefresh = millis();
    Serial.println("--- Données mises à jour ---\n");
}

String buildBitcoinContext() {
    String context = "";

    BitcoinPrice price = bitcoinAPI.getPrice();
    if (price.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Prix: $%.0f USD / %.0f EUR\n", price.usd, price.eur);
        context += buf;
    }

    BitcoinFees fees = bitcoinAPI.getFees();
    if (fees.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Frais: Rapide %d sat/vB, Normal %d sat/vB\n",
                 fees.fastestFee, fees.hourFee);
        context += buf;
    }

    int blockHeight = bitcoinAPI.getBlockHeight();
    if (blockHeight > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "- Bloc: #%d\n", blockHeight);
        context += buf;
    }

    MempoolInfo mempool = bitcoinAPI.getMempoolInfo();
    if (mempool.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Mempool: %d tx (%.1f MB)\n",
                 mempool.count, mempool.vsize / 1000000.0f);
        context += buf;
    }

    HashRateInfo hashrate = bitcoinAPI.getHashRateInfo();
    if (hashrate.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Hashrate: %s\n", bitcoinAPI.formatHashRate().c_str());
        context += buf;
    }

    LightningStats lightning = bitcoinAPI.getLightningStats();
    if (lightning.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Lightning: %d noeuds, %.0f BTC\n",
                 lightning.nodeCount, BitcoinAPI::satsToBTC(lightning.totalCapacity));
        context += buf;
    }

    // Ajouter solde LNbits si configuré
    WalletBalance lnBalance = lnbitsAPI.getBalance();
    if (lnBalance.valid) {
        char buf[128];
        snprintf(buf, sizeof(buf), "- Wallet LNbits: %s\n", lnbitsAPI.formatBalance().c_str());
        context += buf;
    }

    return context;
}

void processVoiceCommand() {
    // Afficher l'état d'écoute
    display.showListening();
    currentState = STATE_COMMAND_LISTENING;

    Serial.println("Parlez maintenant...");

    // Enregistrement avec VAD (arrêt automatique après silence)
    // Max 10 secondes, arrêt après 800ms de silence
    if (!audioManager.startRecordingWithVAD(10000, 800)) {
        Serial.println("Erreur: impossible de démarrer l'enregistrement");
        display.showError("Erreur micro");
        delay(2000);
        currentState = STATE_READY;
        return;
    }

    // L'enregistrement s'arrête automatiquement quand l'utilisateur finit de parler
    size_t audioSize = audioManager.getRecordingSize();
    Serial.printf("Enregistrement terminé: %d bytes\n", audioSize);

    if (audioSize < 2000) {
        Serial.println("Audio trop court");
        display.showError("Parlez plus longtemps");
        delay(2000);
        currentState = STATE_READY;
        return;
    }

    // Transcrire l'audio
    display.showThinking();
    currentState = STATE_TRANSCRIBING;

    String transcription;
    if (!whisperAPI.transcribe(audioManager.getRecordingBuffer(), audioSize, transcription)) {
        Serial.println("Erreur transcription: " + whisperAPI.getLastError());
        display.showError("Erreur transcription");
        delay(2000);
        currentState = STATE_READY;
        return;
    }

    Serial.println("Transcription: " + transcription);

    // Vérifier si c'est du silence (transcription vide ou juste des points/espaces)
    String trimmed = transcription;
    trimmed.trim();
    // Retirer les points et espaces
    String cleaned = "";
    for (int i = 0; i < trimmed.length(); i++) {
        char c = trimmed.charAt(i);
        if (c != '.' && c != ' ' && c != ',') {
            cleaned += c;
        }
    }

    if (cleaned.length() < 2) {
        // Silence détecté - sortir du mode dialogue
        Serial.println("Silence détecté - fin du dialogue");
        display.showMessage("SATOSHI", "À bientôt!");
        delay(1000);
        currentState = STATE_READY;
        return;
    }

    // Vérifier si c'est juste le wake word seul
    String lowerTranscription = transcription;
    lowerTranscription.toLowerCase();
    if (lowerTranscription == "satoshi" || lowerTranscription == "satoshi." ||
        lowerTranscription == "satoshi!" || lowerTranscription == "satoshi?") {
        // C'était juste le wake word, attendre la commande
        display.showMessage("SATOSHI", "Je vous écoute...");
        delay(500);
        processVoiceCommand();  // Réécouter pour la vraie commande
        return;
    }

    // Retirer "satoshi" du début si présent
    if (lowerTranscription.startsWith("satoshi ")) {
        transcription = transcription.substring(8);
    } else if (lowerTranscription.startsWith("satoshi, ")) {
        transcription = transcription.substring(9);
    }

    // Envoyer a Claude
    askClaude(transcription);
}

// Parser et exécuter les actions de la réponse Claude
bool executeClaudeAction(const String& response) {
    bool actionExecuted = false;

    // === ACTION:PAY - Paiement Lightning ===
    int actionIdx = response.indexOf("ACTION:PAY:");
    if (actionIdx >= 0) {
        int lineEnd = response.indexOf('\n', actionIdx);
        if (lineEnd < 0) lineEnd = response.length();

        String actionLine = response.substring(actionIdx + 11, lineEnd);  // Apres "ACTION:PAY:"
        actionLine.trim();

        // Parser address:amount
        int colonIdx = actionLine.lastIndexOf(':');
        if (colonIdx > 0) {
            String address = actionLine.substring(0, colonIdx);
            String amountStr = actionLine.substring(colonIdx + 1);
            int64_t amount = amountStr.toInt();

            Serial.printf("ACTION PAIEMENT: %s -> %lld sats\n", address.c_str(), amount);

            if (amount > 0 && amount <= 100000 && address.length() >= 5) {
                // Afficher confirmation
                char msg[100];
                snprintf(msg, sizeof(msg), "Envoi %lld sats\na %s", amount, address.c_str());
                display.showMessage("PAIEMENT", msg);
                delay(1000);

                // Exécuter le paiement
                bool success = false;
                if (address.startsWith("lnbc") || address.startsWith("LNBC")) {
                    success = lnbitsAPI.payInvoice(address.c_str());
                } else if (address.indexOf('@') > 0) {
                    success = lnbitsAPI.payLnAddress(address.c_str(), amount);
                } else {
                    display.showError("Format adresse?");
                    delay(2000);
                }

                if (success) {
                    Serial.println("Paiement réussi!");
                    display.showMessage("PAYÉ!", "Paiement envoyé");
                    delay(2000);
                    actionExecuted = true;
                } else if (lnbitsAPI.getLastError().length() > 0) {
                    display.showError(lnbitsAPI.getLastError().c_str());
                    delay(3000);
                }
            } else {
                display.showError("Montant invalide");
                delay(2000);
            }
        }
    }

    // === ACTION:RECEIVE - Recevoir des sats avec QR code ===
    actionIdx = response.indexOf("ACTION:RECEIVE:");
    if (actionIdx >= 0) {
        int lineEnd = response.indexOf('\n', actionIdx);
        if (lineEnd < 0) lineEnd = response.length();

        String amountStr = response.substring(actionIdx + 15, lineEnd);
        amountStr.trim();
        int64_t amount = amountStr.toInt();

        Serial.printf("ACTION RECEIVE: %lld sats\n", amount);

        if (amount <= 0) amount = 21;  // Default 21 sats si pas de montant

        display.showMessage("RECEIVE", "Création invoice...");

        String bolt11;
        if (lnbitsAPI.createInvoice(amount, "SATOSHI Receive", bolt11)) {
            Serial.println("Invoice: " + bolt11);
            display.showInvoice(bolt11.c_str(), amount);

            // Jouer le TTS immédiatement après affichage du QR
            char ttsMsg[100];
            snprintf(ttsMsg, sizeof(ttsMsg), "Voici ton invoice Lightning de %lld sats!", amount);
            uint8_t* ttsBuffer = nullptr;
            size_t ttsSize = 0;
            if (speakText(ttsMsg, &ttsBuffer, &ttsSize)) {
                audioManager.playAudio(ttsBuffer, ttsSize);
                free(ttsBuffer);
            }

            // Attendre toucher pour fermer
            unsigned long startWait = millis();
            while (millis() - startWait < 120000) {  // 2 min timeout
                if (touch.touched()) {
                    delay(200);
                    break;
                }
                delay(100);
            }
            actionExecuted = true;
        } else {
            display.showError(lnbitsAPI.getLastError().c_str());
            delay(3000);
        }
    }

    // === ACTION:STATS - Afficher les statistiques ===
    actionIdx = response.indexOf("ACTION:STATS");
    if (actionIdx >= 0) {
        Serial.println("ACTION STATS");

        display.showMessage("STATS", "Chargement...");

        if (lnbitsAPI.fetchStats()) {
            WalletStats stats = lnbitsAPI.getStats();
            char statsMsg[200];
            snprintf(statsMsg, sizeof(statsMsg),
                     "7 jours:\n+%lld / -%lld sats\n(%d tx)\n\n30 jours:\n+%lld / -%lld sats\n(%d tx)",
                     stats.received7d, stats.spent7d, stats.txCount7d,
                     stats.received30d, stats.spent30d, stats.txCount30d);
            display.showMessage("WALLET STATS", statsMsg);

            // Attendre toucher
            unsigned long startWait = millis();
            while (millis() - startWait < 10000) {
                if (touch.touched()) {
                    delay(200);
                    break;
                }
                delay(100);
            }
            actionExecuted = true;
        } else {
            display.showError(lnbitsAPI.getLastError().c_str());
            delay(3000);
        }
    }

    // === ACTION:MINING_STATUS - Afficher statut mining Bitaxe ===
    actionIdx = response.indexOf("ACTION:MINING_STATUS");
    if (actionIdx >= 0) {
        Serial.println("ACTION MINING_STATUS");

        // Rafraîchir les données mining si nécessaire
        if (miningManager.needsRefresh()) {
            display.showMessage("MINING", "Rafraîchissement...");
            miningManager.refreshAll();
        }

        // Afficher les stats mining
        display.showMiningStats();

        // TTS dès l'affichage des stats
        if (miningManager.getMinerCount() > 0) {
            MiningStats stats = miningManager.getAggregatedStats();
            char ttsMsg[200];
            snprintf(ttsMsg, sizeof(ttsMsg),
                "Mining: %d mineurs en ligne. Hashrate total %s. Consommation %.1f watts. Température moyenne %.0f degrés.",
                stats.onlineCount, miningManager.formatTotalHashrate().c_str(),
                stats.totalPower, stats.avgTemp);

            uint8_t* ttsBuffer = nullptr;
            size_t ttsSize = 0;
            if (speakText(ttsMsg, &ttsBuffer, &ttsSize)) {
                audioManager.playAudio(ttsBuffer, ttsSize);
                free(ttsBuffer);
            }
        } else {
            uint8_t* ttsBuffer = nullptr;
            size_t ttsSize = 0;
            if (speakText("Aucun mineur configuré.", &ttsBuffer, &ttsSize)) {
                audioManager.playAudio(ttsBuffer, ttsSize);
                free(ttsBuffer);
            }
        }

        // Attendre toucher
        unsigned long startWait = millis();
        while (millis() - startWait < 10000) {
            if (touch.touched()) {
                delay(200);
                break;
            }
            delay(100);
        }
        actionExecuted = true;
    }

    return actionExecuted;
}

void askClaude(const String& question) {
    lastUserMessage = question;
    currentState = STATE_PROCESSING;
    dialogueInterrupted = false;

    if (millis() - lastDataRefresh > DATA_REFRESH_INTERVAL) {
        refreshBitcoinData();
    }

    // Mettre à jour le contexte mining si des mineurs sont configurés
    if (miningManager.getMinerCount() > 0) {
        if (miningManager.needsRefresh()) {
            miningManager.refreshAll();
        }
        claudeAPI.updateMiningContext(miningManager.formatMiningStatus());
    }

    display.showThinking();
    Serial.println("Envoi a Claude...");

    String response;
    if (claudeAPI.sendMessage(question.c_str(), response)) {
        // Vérifier interruption touch
        if (touch.touched()) {
            Serial.println("Touch - retour menu");
            dialogueInterrupted = true;
            currentState = STATE_READY;
            display.showSatoshiReady();
            delay(200);
            return;
        }

        // Vérifier s'il y a une action à exécuter
        bool actionExecuted = executeClaudeAction(response);

        // Si action exécutée, vérifier touch avant de continuer
        if (actionExecuted && touch.touched()) {
            dialogueInterrupted = true;
            currentState = STATE_READY;
            display.showSatoshiReady();
            delay(200);
            return;
        }

        // Retirer la ligne ACTION de la réponse affichée
        String displayResponse = response;
        int actionIdx = displayResponse.indexOf("ACTION:");
        if (actionIdx >= 0) {
            int lineEnd = displayResponse.indexOf('\n', actionIdx);
            if (lineEnd >= 0) {
                displayResponse = displayResponse.substring(lineEnd + 1);
            } else {
                displayResponse = "";
            }
        }
        displayResponse.trim();

        lastResponse = displayResponse;
        Serial.println("\n--- Réponse SATOSHI ---");
        Serial.println(displayResponse);
        Serial.println("------------------------\n");

        display.showConversation(lastUserMessage.c_str(), lastResponse.c_str());

        // === Lecture audio TTS avec interruption possible ===
        uint8_t* ttsBuffer = nullptr;
        size_t ttsSize = 0;
        unsigned long audioDurationMs = 0;

        if (speakText(lastResponse.c_str(), &ttsBuffer, &ttsSize)) {
            Serial.printf("Lecture audio TTS (%d bytes)...\n", ttsSize);

            // Calculer la durée de l'audio WAV
            if (ttsSize > 44) {
                uint32_t sampleRate = ttsBuffer[24] | (ttsBuffer[25] << 8) |
                                      (ttsBuffer[26] << 16) | (ttsBuffer[27] << 24);
                size_t pcmSize = ttsSize - 44;
                audioDurationMs = (pcmSize * 1000) / (sampleRate * 2);
            }

            audioManager.playAudio(ttsBuffer, ttsSize);
            free(ttsBuffer);

            // Attendre avec vérification touch
            if (audioDurationMs > 0) {
                if (delayWithTouchCheck(audioDurationMs + 300)) {
                    // Touch détecté pendant audio - retour menu
                    currentState = STATE_READY;
                    display.showSatoshiReady();
                    return;
                }
            }
        } else {
            // TTS échoué - vérifier si rate limit
            if (ttsRateLimitHit) {
                char rateLimitMsg[100];
                snprintf(rateLimitMsg, sizeof(rateLimitMsg), "TTS limite!\n%d/%d tokens\nRetry: %s",
                         ttsRateLimitUsed, ttsRateLimitTotal, ttsRateLimitRetryTime.c_str());
                Serial.println(rateLimitMsg);
                display.showError(rateLimitMsg);
                if (delayWithTouchCheck(2000)) {
                    currentState = STATE_READY;
                    display.showSatoshiReady();
                    return;
                }
                display.showConversation(lastUserMessage.c_str(), lastResponse.c_str());
            }
        }

        // Attendre touch ou timeout pour continuer dialogue
        Serial.println("\n*** Touchez pour finir, ou parlez... ***");
        display.showMessage("SATOSHI", "Touchez=fin\nParlez=continue");

        // Attendre 1.5 sec avec vérification touch
        if (delayWithTouchCheck(1500)) {
            // Touch détecté - fin dialogue
            currentState = STATE_READY;
            display.showSatoshiReady();
            return;
        }

        // Continuer le dialogue
        display.showMessage("SATOSHI", "Je t'écoute...");

        if (delayWithTouchCheck(300)) {
            currentState = STATE_READY;
            display.showSatoshiReady();
            return;
        }

        audioManager.playTestTone(880, 50);

        if (delayWithTouchCheck(100)) {
            currentState = STATE_READY;
            display.showSatoshiReady();
            return;
        }

        processVoiceCommand();
        return;

    } else {
        Serial.println("Erreur: " + claudeAPI.getLastError());
        display.showError(claudeAPI.getLastError().c_str());
        delay(3000);
    }

    currentState = STATE_READY;
}

void loop() {
    switch (currentState) {

        case STATE_SETUP_PORTAL:
            captivePortal.handleClient();

            if (captivePortal.isConfigurationComplete()) {
                Serial.println("Configuration terminée - redémarrage...");
                display.showMessage("OK!", "Redémarrage...");
                delay(2000);
                ESP.restart();
            }
            break;

        case STATE_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                String ip = WiFi.localIP().toString();
                Serial.printf("\nConnecté! IP: %s\n", ip.c_str());
                display.showConnected(ip.c_str());

                // Flash LED verte pour indiquer connexion WiFi
                rgbLed.begin();
                for (int i = 0; i < 3; i++) {
                    rgbLed.setGreen(150);
                    delay(200);
                    rgbLed.off();
                    delay(200);
                }
                delay(400);

                // Initialiser les APIs
                claudeAPI.begin();
                bitcoinAPI.begin();
                lnbitsAPI.begin();
                whisperAPI.begin();
                miningManager.begin();

                // Démarrer l'audio I2S avec ES8311 codec EN PREMIER
                // (initialise Wire/I2C_NUM_0 pour ES8311 et touch)
                Serial.println("Démarrage Audio Manager (I2S + ES8311)...");
                if (!audioManager.begin()) {
                    Serial.println("ERREUR: Audio Manager init échoué!");
                } else {
                    Serial.println("Audio Manager OK");

                    // Mélodie de démarrage courte (volume 50%)
                    Serial.println("Mélodie de démarrage...");
                    audioManager.playTestTone(784, 100);   // G5 - note unique courte
                    Serial.println("Mélodie terminée");

                    Serial.println("\nUtilisez /record pour tester le microphone");
                    Serial.println("Puis /play pour rejouer l'enregistrement");
                }

                // Initialiser le tactile APRÈS audio (Wire déjà initialisé)
                touch.begin();

                // Scanner le bus I2C Wire pour trouver les devices
                Serial.println("Scan I2C Wire...");
                for (uint8_t addr = 1; addr < 127; addr++) {
                    Wire.beginTransmission(addr);
                    if (Wire.endTransmission() == 0) {
                        Serial.printf("  Device I2C trouvé à 0x%02X\n", addr);
                    }
                }

                // Initialiser le wake word detector
                if (useWakeWord) {
                    wakeWord.begin();
                }

                currentState = STATE_FETCHING_DATA;
            }
            else if (millis() - stateTimer > 500) {
                stateTimer = millis();
                wifiRetryCount++;
                Serial.print(".");

                if (wifiRetryCount >= MAX_WIFI_RETRIES) {
                    Serial.println("\nÉchec connexion WiFi");
                    display.showError("WiFi non accessible");
                    delay(3000);

                    configManager.reset();
                    currentState = STATE_SETUP_PORTAL;
                    captivePortal.begin();
                }
            }
            break;

        case STATE_FETCHING_DATA:
            refreshBitcoinData();

            currentState = STATE_READY;
            display.showSatoshiReady();

            Serial.println("\n========================================");
            Serial.println("SATOSHI est prêt!");
            Serial.printf("Bitcoin: %s | Bloc #%d\n",
                          bitcoinAPI.formatPrice("EUR").c_str(),
                          bitcoinAPI.getBlockHeight());
            if (useWakeWord) {
                Serial.println("Dites 'SATOSHI' pour parler");
            } else {
                Serial.println("Appuyez sur le bouton pour parler");
            }
            Serial.println("Tapez une question dans le terminal pour tester");
            Serial.println("========================================\n");

            // Message de bienvenue TTS (en français)
            {
                uint8_t* ttsBuffer = nullptr;
                size_t ttsSize = 0;
                Serial.println("TTS: Bonjour, je suis SATOSHI...");
                if (speakText("Bonjour! Je suis Satoshi, ton assistant Bitcoin. Dis mon nom pour me parler.", &ttsBuffer, &ttsSize)) {
                    audioManager.playAudio(ttsBuffer, ttsSize);
                    free(ttsBuffer);
                }
            }
            break;

        case STATE_READY:
            // Traiter les commandes série (test direct)
            processSerialCommand();

            // Gestion du clavier virtuel
            if (keyboard.isVisible()) {
                if (touch.touched()) {
                    int16_t tx, ty;
                    touch.getPoint(tx, ty);

                    // Traiter le toucher sur le clavier (retourne: -1=RETOUR, 0=rien, 1=OK)
                    int result = keyboard.handleTouch(tx, ty);

                    if (result == -1) {
                        // Bouton RETOUR appuyé - fermer le clavier
                        Serial.println("RETOUR appuyé - fermeture clavier");
                        keyboard.hide();
                        keyboard.clear();
                        display.showSatoshiReady();
                    }
                    else if (result == 1) {
                        // Entrée appuyée - envoyer la question
                        String question = keyboard.getText();
                        if (question.length() > 0) {
                            Serial.printf("Question clavier: %s\n", question.c_str());
                            keyboard.hide();

                            // Afficher la question
                            display.showMessage("VOUS", question.c_str());
                            delay(500);

                            // Envoyer a Claude
                            askClaude(question);

                            // Afficher la réponse
                            display.showResponse(lastResponse.c_str());

                            // Attendre toucher pour fermer
                            delay(500);
                            while (!touch.touched()) {
                                delay(50);
                            }
                        }

                        // Retour écran principal
                        keyboard.clear();
                        display.showSatoshiReady();
                    }

                    delay(150);  // Debounce clavier
                }
            } else {
                // Gestion des boutons tactiles (écran principal - mode PAYSAGE 320x240)
                // Boutons sur 2 lignes:
                // Ligne 1 (y=100-145): ECRIRE (x=10-155), INFO (x=165-310)
                // Ligne 2 (y=155-200): INVOICE (x=10-155), CONFIG (x=165-310)
                if (touch.touched()) {
                    int16_t tx, ty;
                    touch.getPoint(tx, ty);
                    Serial.printf("Touch: x=%d, y=%d\n", tx, ty);

                    // Bouton ÉCRIRE: x=10-155, y=100-145
                    if (tx >= 10 && tx <= 155 && ty >= 100 && ty <= 145) {
                        Serial.println("Bouton ÉCRIRE appuyé - ouverture clavier");
                        keyboard.clear();
                        keyboard.show();
                    }
                    // Bouton MINING: x=165-310, y=100-145
                    else if (tx >= 165 && tx <= 310 && ty >= 100 && ty <= 145) {
                        Serial.println("Bouton MINING appuyé!");
                        if (useWakeWord) {
                            wakeWord.pause();
                        }
                        handleMiningMenu();
                        display.showSatoshiReady();
                        if (useWakeWord) {
                            wakeWord.resume();
                        }
                    }
                    // Bouton INVOICE: x=10-155, y=155-200
                    else if (tx >= 10 && tx <= 155 && ty >= 155 && ty <= 200) {
                        Serial.println("Bouton INVOICE appuyé!");

                        // Créer une invoice de 21 sats
                        display.showMessage("INVOICE", "Création 21 sats...");

                        String bolt11;
                        if (lnbitsAPI.createInvoice(21, "SATOSHI 21 sats", bolt11)) {
                            Serial.println("Invoice créée: " + bolt11);
                            // Afficher le QR code et le BOLT11
                            display.showInvoice(bolt11.c_str(), 21);

                            // Attendre paiement ou toucher pour fermer
                            unsigned long startWait = millis();
                            while (millis() - startWait < 60000) {  // 60 sec timeout
                                if (touch.touched()) {
                                    delay(200);
                                    break;
                                }
                                delay(100);
                            }
                        } else {
                            display.showError("Erreur invoice");
                            delay(2000);
                        }

                        display.showSatoshiReady();
                    }
                    // Bouton PARLER: x=165-310, y=155-200 (remplace CONFIG)
                    else if (tx >= 165 && tx <= 310 && ty >= 155 && ty <= 200) {
                        Serial.println("Bouton PARLER appuyé - activation vocale directe");
                        if (useWakeWord) {
                            wakeWord.pause();
                        }
                        display.showMessage("SATOSHI", "Je vous écoute...");
                        delay(300);
                        processVoiceCommand();
                        display.showSatoshiReady();
                        if (useWakeWord) {
                            wakeWord.resume();
                        }
                    }

                    delay(200);  // Debounce
                }

                // Clignement aléatoire des yeux du masque
                if (millis() - lastBlinkTime > nextBlinkDelay) {
                    lastBlinkTime = millis();
                    // Prochain clignement dans 3 à 8 secondes
                    nextBlinkDelay = 3000 + random(5000);
                    // Clignement du masque mini (position: cx=50, cy=50, size=55)
                    guyFawkes.randomBlink(display.getGfx(), 50, 50, 55);
                }
            }

            // Mode Wake Word
            if (useWakeWord) {
                // Détection du wake word
                if (wakeWord.detect()) {
                    Serial.println("\n*** WAKE WORD DÉTECTÉ! ***");
                    wakeWord.pause();

                    // Afficher confirmation
                    display.showMessage("SATOSHI", "Oui?");
                    delay(300);

                    // Écouter la commande
                    processVoiceCommand();

                    // Reprendre l'écoute du wake word
                    display.showSatoshiReady();
                    wakeWord.resume();
                }

                // Afficher le niveau audio périodiquement
                if (millis() - lastLevelUpdate > 100) {
                    lastLevelUpdate = millis();
                    // Optionnel: afficher niveau audio sur l'ecran
                    // display.showVolumeLevel(wakeWord.getAudioLevel());
                }
            }

            // Bouton BOOT: appui court = parler, appui long (3s) = reset config
            if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
                buttonPressed = true;
                unsigned long pressStart = millis();
                delay(50);  // Debounce

                if (digitalRead(BUTTON_PIN) == LOW) {
                    // Attendre release ou long press
                    bool longPress = false;
                    while (digitalRead(BUTTON_PIN) == LOW) {
                        if (millis() - pressStart > 3000) {
                            // Long press détecté - reset config
                            longPress = true;
                            display.showMessage("RESET", "Config effacée...");
                            rgbLed.setRed(150);
                            delay(500);
                            configManager.reset();
                            rgbLed.off();
                            display.showMessage("OK", "Redémarrage...");
                            delay(1000);
                            ESP.restart();
                        }
                        // Afficher feedback visuel après 1.5s
                        if (millis() - pressStart > 1500 && millis() - pressStart < 1600) {
                            display.showMessage("BOOT", "Maintenir 3s\npour RESET");
                        }
                        delay(50);
                    }

                    // Appui court - activer commande vocale
                    if (!longPress) {
                        if (useWakeWord) {
                            wakeWord.pause();
                        }

                        display.showMessage("SATOSHI", "Je vous écoute...");
                        delay(300);
                        processVoiceCommand();

                        display.showSatoshiReady();
                        if (useWakeWord) {
                            wakeWord.resume();
                        }
                    }
                }
            }

            if (digitalRead(BUTTON_PIN) == HIGH) {
                buttonPressed = false;
            }

            // Vérifier la connexion WiFi
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi déconnecté - reconnexion...");
                if (useWakeWord) {
                    wakeWord.pause();
                }
                currentState = STATE_CONNECTING;
                connectToWiFi();
            }

            // Rafraîchir les données périodiquement
            if (millis() - lastDataRefresh > DATA_REFRESH_INTERVAL * 5) {
                bitcoinAPI.fetchPrice();
                bitcoinAPI.fetchFees();
                bitcoinAPI.fetchBlockHeight();
                String context = buildBitcoinContext();
                claudeAPI.updateBitcoinContext(context);
                lastDataRefresh = millis();
            }
            break;

        case STATE_WAKE_LISTENING:
        case STATE_COMMAND_LISTENING:
        case STATE_TRANSCRIBING:
        case STATE_PROCESSING:
        case STATE_SPEAKING:
            // Géré dans les fonctions respectives
            break;

        case STATE_ERROR:
            delay(100);
            break;

        default:
            break;
    }

    delay(10);
}

// Traitement des commandes série pour test direct
void processSerialCommand() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                Serial.println("\n>>> Commande série: " + serialBuffer);

                // Commandes spéciales de debug
                if (serialBuffer == "/mic" || serialBuffer == "/testmic") {
                    Serial.println("Test du micro GPIO2 (L420A7J1)...");
                    audioManager.testMicGPIO2();
                    display.showSatoshiReady();
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/scanpdm") {
                    Serial.println("Scan des pins PDM...");
                    audioManager.scanPDMPins();
                    display.showSatoshiReady();
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/scanamp") {
                    Serial.println("Scan des pins ampli...");
                    audioManager.scanI2SPins();
                    display.showSatoshiReady();
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/testmicin" || serialBuffer == "/mic1" ||
                         serialBuffer == "/mic2" || serialBuffer == "/micboth") {
                    Serial.println("Commandes MIC non disponibles avec driver Freenove");
                    Serial.println("Utilisez /record pour tester le microphone");
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/play") {
                    // Rejouer l'audio enregistré via le speaker
                    Serial.println("Lecture audio enregistré via speaker...");
                    audioManager.playRecordedAudio();
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/record") {
                    // Enregistrer et afficher les statistiques détaillées
                    Serial.println("\n=== ENREGISTREMENT TEST (5 sec) ===");
                    Serial.println("Parlez pendant l'enregistrement...");
                    audioManager.startRecording();

                    uint8_t* buffer = audioManager.getRecordingBuffer();
                    size_t size = audioManager.getRecordingSize();
                    int16_t* samples = (int16_t*)buffer;
                    int numSamples = size / 2;

                    // Analyser le contenu
                    int32_t sum = 0;
                    int32_t sumAbs = 0;
                    int zeroCount = 0;
                    int silentCount = 0;  // samples < 100

                    for (int i = 0; i < numSamples; i++) {
                        sum += samples[i];
                        sumAbs += abs(samples[i]);
                        if (samples[i] == 0) zeroCount++;
                        if (abs(samples[i]) < 100) silentCount++;
                    }

                    Serial.printf("Samples: %d\n", numSamples);
                    Serial.printf("Moyenne: %d\n", (int)(sum / numSamples));
                    Serial.printf("Moyenne abs: %d\n", (int)(sumAbs / numSamples));
                    Serial.printf("Zeros: %d (%.1f%%)\n", zeroCount, 100.0f * zeroCount / numSamples);
                    Serial.printf("Silencieux (<100): %d (%.1f%%)\n", silentCount, 100.0f * silentCount / numSamples);

                    // Afficher les 50 premiers samples
                    Serial.println("\nPremiers 50 samples:");
                    for (int i = 0; i < 50 && i < numSamples; i++) {
                        Serial.printf("%6d ", samples[i]);
                        if ((i + 1) % 10 == 0) Serial.println();
                    }

                    // Afficher quelques samples au milieu (quand on parle)
                    int midStart = numSamples / 2 - 25;
                    Serial.printf("\nSamples milieu (%d-%d):\n", midStart, midStart + 50);
                    for (int i = midStart; i < midStart + 50 && i < numSamples; i++) {
                        Serial.printf("%6d ", samples[i]);
                        if ((i - midStart + 1) % 10 == 0) Serial.println();
                    }

                    Serial.println("\n=== FIN ENREGISTREMENT TEST ===\n");
                    serialBuffer = "";
                    return;
                }
                else if (serialBuffer == "/help") {
                    Serial.println("\n=== COMMANDES DISPONIBLES ===");
                    Serial.println("/mic       - Test micro GPIO2 (L420A7J1)");
                    Serial.println("/record    - Enregistrer et analyser audio");
                    Serial.println("/play      - Écouter l'audio enregistré via speaker");
                    Serial.println("/testmicin - Test entrées MIC ES8311");
                    Serial.println("/mic1      - Sélectionner MIC1 seul");
                    Serial.println("/mic2      - Sélectionner MIC2 seul");
                    Serial.println("/micboth   - Sélectionner MIC1+MIC2");
                    Serial.println("/scanpdm   - Scan complet pins PDM");
                    Serial.println("/scanamp   - Scan pins amplificateur");
                    Serial.println("/help      - Cette aide");
                    Serial.println("Autre      - Envoyer à Claude");
                    Serial.println("=============================\n");
                    serialBuffer = "";
                    return;
                }

                if (useWakeWord) {
                    wakeWord.pause();
                }

                // Envoyer directement a Claude (bypass Whisper)
                askClaude(serialBuffer);

                display.showSatoshiReady();
                if (useWakeWord) {
                    wakeWord.resume();
                }

                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
}

// ============================================================
// Menu Mining - Gestion des mineurs Bitaxe
// ============================================================
void handleMiningMenu() {
    inMiningMenu = true;
    display.showMiningMenu();

    while (inMiningMenu) {
        if (touch.touched()) {
            int16_t tx, ty;
            touch.getPoint(tx, ty);
            Serial.printf("Mining touch: x=%d, y=%d\n", tx, ty);

            // Bouton STATS: x=10-155, y=50-90
            if (tx >= 10 && tx <= 155 && ty >= 50 && ty <= 90) {
                Serial.println("Mining: STATS");
                display.showMiningStats();

                // TTS dès l'affichage des stats
                if (miningManager.getMinerCount() > 0) {
                    MiningStats stats = miningManager.getAggregatedStats();
                    char ttsMsg[200];
                    snprintf(ttsMsg, sizeof(ttsMsg),
                        "Mining: %d mineurs en ligne. Hashrate total %s. Consommation %.1f watts. Température moyenne %.0f degrés.",
                        stats.onlineCount, miningManager.formatTotalHashrate().c_str(),
                        stats.totalPower, stats.avgTemp);

                    uint8_t* ttsBuffer = nullptr;
                    size_t ttsSize = 0;
                    if (speakText(ttsMsg, &ttsBuffer, &ttsSize)) {
                        audioManager.playAudio(ttsBuffer, ttsSize);
                        free(ttsBuffer);
                    }
                }

                // Attendre toucher pour fermer
                while (!touch.touched()) { delay(50); }
                delay(200);
                display.showMiningMenu();
            }
            // Bouton LISTE: x=165-310, y=50-90
            else if (tx >= 165 && tx <= 310 && ty >= 50 && ty <= 90) {
                Serial.println("Mining: LISTE");
                display.showMinersList();

                // Attendre toucher pour fermer
                while (!touch.touched()) { delay(50); }
                delay(200);
                display.showMiningMenu();
            }
            // Bouton AJOUTER: x=10-155, y=100-140
            else if (tx >= 10 && tx <= 155 && ty >= 100 && ty <= 140) {
                Serial.println("Mining: AJOUTER");

                // Afficher l'écran d'ajout et le clavier
                display.showAddMinerKeyboard();
                delay(300);

                keyboard.clear();
                keyboard.show();

                // Attendre l'entrée de l'IP
                bool done = false;
                while (!done) {
                    if (touch.touched()) {
                        int16_t kx, ky;
                        touch.getPoint(kx, ky);

                        int result = keyboard.handleTouch(kx, ky);

                        if (result == -1) {
                            // RETOUR
                            done = true;
                            keyboard.hide();
                            keyboard.clear();
                        }
                        else if (result == 1) {
                            // OK - ajouter le mineur
                            String ip = keyboard.getText();
                            if (ip.length() > 0) {
                                display.showMessage("MINING", "Connexion...");
                                keyboard.hide();

                                if (miningManager.addMiner(ip.c_str())) {
                                    display.showMessage("OK!", "Mineur ajouté!");
                                    delay(1500);
                                } else {
                                    display.showError("Erreur connexion");
                                    delay(2000);
                                }
                            }
                            done = true;
                            keyboard.clear();
                        }

                        delay(150);
                    }
                    delay(20);
                }

                display.showMiningMenu();
            }
            // Bouton REFRESH: x=165-310, y=100-140
            else if (tx >= 165 && tx <= 310 && ty >= 100 && ty <= 140) {
                Serial.println("Mining: REFRESH");
                display.showMessage("MINING", "Rafraîchissement...");

                miningManager.refreshAll();

                display.showMessage("OK!", "Données mises à jour");
                delay(1000);
                display.showMiningMenu();
            }
            // Bouton RETOUR: x=10-310, y=150-190
            else if (tx >= 10 && tx <= 310 && ty >= 150 && ty <= 190) {
                Serial.println("Mining: RETOUR");
                inMiningMenu = false;
            }

            delay(200);  // Debounce
        }

        delay(20);
    }
}
