// wake_word.cpp - Détection du wake word "SATOSHI"
// Pour Freenove ESP32-S3 2.8" avec ES8311 codec
// Utilise Whisper API pour transcrire et détecter "SATOSHI"
#include "wake_word.h"
#include "audio.h"  // Pour accéder à i2s_audio
#include "whisper_api.h"  // Pour transcription
#include <math.h>

WakeWordDetector wakeWord;

WakeWordDetector::WakeWordDetector() {
    listening = false;
    state = WW_IDLE;
    audioBuffer = nullptr;
    audioSize = 0;
    bufferCapacity = 0;
    speechFrameCount = 0;
    silenceFrameCount = 0;
    currentLevel = 0;
    lastDetectionTime = 0;
    speechStartTime = 0;
}

bool WakeWordDetector::begin() {
    // Allouer le buffer en PSRAM si disponible
    bufferCapacity = 16000 * 2 * 5;  // 5 secondes à 16kHz 16bit

    if (psramFound()) {
        audioBuffer = (uint8_t*)ps_malloc(bufferCapacity);
        Serial.println("Wake word buffer alloué en PSRAM");
    } else {
        audioBuffer = (uint8_t*)malloc(bufferCapacity);
        Serial.println("Wake word buffer alloué en RAM");
    }

    if (!audioBuffer) {
        Serial.println("Erreur: impossible d'allouer le buffer wake word");
        return false;
    }

    Serial.println("Wake Word Detector initialisé");
    Serial.println("Dites 'SATOSHI' pour activer l'assistant");

    return true;
}

void WakeWordDetector::end() {
    pause();
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
}

bool WakeWordDetector::initMicrophone() {
    // L'I2S est déjà initialisé par audioManager
    // On utilise l'instance i2s_audio partagée
    // Audio amplifier enable (active LOW)
    if (PIN_AUDIO_EN >= 0) {
        pinMode(PIN_AUDIO_EN, OUTPUT);
        digitalWrite(PIN_AUDIO_EN, LOW);
    }

    Serial.println("Wake word: utilise I2S partagé avec audioManager");
    return true;
}

void WakeWordDetector::deinitMicrophone() {
    // Ne pas désinstaller I2S car il est partagé avec audioManager
}

void WakeWordDetector::pause() {
    if (listening) {
        listening = false;
        state = WW_IDLE;
    }
}

void WakeWordDetector::resume() {
    if (!listening) {
        if (initMicrophone()) {
            listening = true;
            state = WW_LISTENING;
            speechFrameCount = 0;
            silenceFrameCount = 0;
            audioSize = 0;
        }
    }
}

int WakeWordDetector::calculateEnergy(int16_t* samples, size_t count) {
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t sample = samples[i];
        sum += sample * sample;
    }
    return (int)sqrt((double)(sum / count));
}

bool WakeWordDetector::isSpeech(int energy) {
    return energy > ENERGY_THRESHOLD;
}

bool WakeWordDetector::detect() {
    if (!listening) {
        resume();
        if (!listening) return false;
    }

    // Lire un chunk audio via ESP_I2S
    int16_t samples[AUDIO_CHUNK_SIZE];
    size_t bytesRead = 0;

    // Utiliser readBytes de I2SClass
    bytesRead = i2s_audio.readBytes((char*)samples, sizeof(samples));
    if (bytesRead == 0) {
        return false;
    }

    size_t sampleCount = bytesRead / sizeof(int16_t);

    // Calculer l'énergie
    int energy = calculateEnergy(samples, sampleCount);
    currentLevel = map(energy, 0, 10000, 0, 100);
    if (currentLevel > 100) currentLevel = 100;

    bool hasSpeech = isSpeech(energy);

    switch (state) {
        case WW_LISTENING:
            if (hasSpeech) {
                speechFrameCount++;
                silenceFrameCount = 0;

                // Début de parole détecté
                if (speechFrameCount >= SPEECH_FRAMES_REQUIRED) {
                    Serial.println("Parole détectée - enregistrement...");
                    state = WW_DETECTED;
                    speechStartTime = millis();
                    audioSize = 0;
                }
            } else {
                speechFrameCount = 0;
                silenceFrameCount++;
            }
            break;

        case WW_DETECTED:
            // Enregistrer l'audio pendant la parole
            if (audioSize + bytesRead < bufferCapacity) {
                memcpy(audioBuffer + audioSize, samples, bytesRead);
                audioSize += bytesRead;
            }

            if (hasSpeech) {
                silenceFrameCount = 0;
            } else {
                silenceFrameCount++;

                // Fin de parole (silence prolongé)
                if (silenceFrameCount >= SILENCE_FRAMES_REQUIRED) {
                    unsigned long duration = millis() - speechStartTime;
                    Serial.printf("Fin parole - durée: %lu ms, taille: %d bytes\n",
                                  duration, audioSize);

                    // Vérifier si c'est assez long pour être un wake word
                    // "SATOSHI" prend environ 600-2500ms à prononcer
                    if (duration >= MIN_WAKE_WORD_DURATION && duration <= MAX_WAKE_WORD_DURATION && audioSize > 10000) {
                        // Transcrire l'audio avec Whisper pour vérifier le wake word
                        Serial.println("Transcription pour détection wake word...");
                        String transcription;
                        // Prompt strict pour guider Whisper vers "Satoshi" uniquement
                        const char* wakeWordPrompt = "Satoshi Nakamoto, hey Satoshi, OK Satoshi";
                        // Utiliser anglais ("en") car "Satoshi" est mieux reconnu en anglais
                        if (whisperAPI.transcribeWithPrompt(audioBuffer, audioSize, transcription, wakeWordPrompt, "en")) {
                            transcription.toLowerCase();
                            transcription.trim();
                            Serial.printf("Wake word check: \"%s\"\n", transcription.c_str());

                            // Vérification STRICTE - uniquement les variantes proches de "satoshi"
                            // Doit contenir "sato" ET se terminer par "shi/chi/si/sy"
                            bool isWakeWord = false;

                            // Variantes acceptées (strictes)
                            if (transcription.indexOf("satoshi") >= 0 ||
                                transcription.indexOf("satoushi") >= 0 ||
                                transcription.indexOf("satochi") >= 0 ||
                                transcription.indexOf("satosi") >= 0) {
                                isWakeWord = true;
                            }

                            // Vérification supplémentaire: le mot doit être relativement court
                            // (éviter les phrases longues qui contiennent "satoshi" par hasard)
                            if (isWakeWord && transcription.length() > 30) {
                                // Si la transcription est longue, vérifier que "satoshi" est au début
                                if (transcription.indexOf("satoshi") > 10 &&
                                    transcription.indexOf("satoushi") > 10 &&
                                    transcription.indexOf("satochi") > 10) {
                                    Serial.println("Wake word trop loin dans la phrase - ignoré");
                                    isWakeWord = false;
                                }
                            }

                            if (isWakeWord) {
                                Serial.println("*** WAKE WORD 'SATOSHI' CONFIRMÉ! ***");
                                state = WW_CONFIRMED;
                                lastDetectionTime = millis();
                                return true;  // Wake word confirmé!
                            } else {
                                Serial.printf("Pas de wake word détecté (transcription: %s)\n", transcription.c_str());
                                state = WW_LISTENING;
                                audioSize = 0;
                                speechFrameCount = 0;
                            }
                        } else {
                            Serial.println("Erreur transcription wake word");
                            state = WW_LISTENING;
                            audioSize = 0;
                            speechFrameCount = 0;
                        }
                    } else {
                        // Trop court ou trop long, reset
                        if (duration < MIN_WAKE_WORD_DURATION) {
                            Serial.printf("Parole trop courte (%lu ms) - ignorée\n", duration);
                        } else if (duration > MAX_WAKE_WORD_DURATION) {
                            Serial.printf("Parole trop longue (%lu ms) - ignorée\n", duration);
                        }
                        state = WW_LISTENING;
                        audioSize = 0;
                        speechFrameCount = 0;
                    }
                }
            }

            // Timeout si parole trop longue
            if (millis() - speechStartTime > 5000) {
                Serial.println("Timeout parole - reset");
                state = WW_LISTENING;
                audioSize = 0;
                speechFrameCount = 0;
            }
            break;

        case WW_CONFIRMED:
            // Attendre un peu avant de reprendre l'écoute
            if (millis() - lastDetectionTime > 1000) {
                state = WW_LISTENING;
                speechFrameCount = 0;
                silenceFrameCount = 0;
            }
            break;

        default:
            state = WW_LISTENING;
            break;
    }

    return false;
}
