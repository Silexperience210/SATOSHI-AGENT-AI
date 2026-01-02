// wake_word.h - Detection du wake word "SATOSHI"
#ifndef WAKE_WORD_H
#define WAKE_WORD_H

#include <Arduino.h>
#include "config.h"

// Configuration du wake word
#define WAKE_WORD "SATOSHI"
#define WAKE_WORD_TIMEOUT 5000    // Timeout entre detections (ms)
#define AUDIO_CHUNK_SIZE 512      // Taille du buffer audio
#define ENERGY_THRESHOLD 1000     // Seuil d'energie pour detection voix (augmente avec gain 42dB)
#define SPEECH_FRAMES_REQUIRED 4  // Frames consecutives pour confirmer parole (plus strict)
#define SILENCE_FRAMES_REQUIRED 15 // Frames de silence pour fin de parole
#define MIN_WAKE_WORD_DURATION 600  // Duree min pour "SATOSHI" en ms
#define MAX_WAKE_WORD_DURATION 2500 // Duree max pour "SATOSHI" en ms

// Etats de detection
enum WakeWordState {
    WW_IDLE,           // En attente
    WW_LISTENING,      // Ecoute active pour wake word
    WW_DETECTED,       // Wake word potentiellement detecte
    WW_CONFIRMED       // Wake word confirme
};

class WakeWordDetector {
public:
    WakeWordDetector();

    bool begin();
    void end();

    // Detection en continu - retourne true si wake word detecte
    bool detect();

    // Arreter la detection temporairement
    void pause();
    void resume();

    // Obtenir l'audio enregistre apres le wake word
    uint8_t* getAudioBuffer() { return audioBuffer; }
    size_t getAudioSize() { return audioSize; }

    // Niveau sonore actuel (0-100)
    int getAudioLevel() { return currentLevel; }

    // Etat actuel
    WakeWordState getState() { return state; }

    bool isListening() { return listening; }

private:
    bool listening;
    WakeWordState state;

    // Buffer audio pour enregistrement apres wake word
    uint8_t* audioBuffer;
    size_t audioSize;
    size_t bufferCapacity;

    // Detection de parole
    int speechFrameCount;
    int silenceFrameCount;
    int currentLevel;

    // Timing
    unsigned long lastDetectionTime;
    unsigned long speechStartTime;

    // I2S pour microphone
    bool initMicrophone();
    void deinitMicrophone();

    // Analyse audio
    int calculateEnergy(int16_t* samples, size_t count);
    bool isSpeech(int energy);
};

extern WakeWordDetector wakeWord;

#endif
