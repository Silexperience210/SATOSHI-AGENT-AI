// audio.h - Gestion audio pour Freenove ESP32-S3 2.8" (FNK0104)
// Utilise ES8311 codec via I2S avec bibliothèque ESP_I2S
#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <ESP_I2S.h>
#include "config.h"

class AudioManager {
public:
    AudioManager();

    bool begin();
    void end();

    // Enregistrement (via ES8311 ADC)
    bool startRecording();  // Enregistrement fixe 5 secondes
    bool startRecordingWithVAD(int maxDurationMs = 10000, int silenceMs = 800);  // Avec detection fin de parole
    void stopRecording();
    bool isRecording() { return recording; }
    int getRecordingDuration();

    // Lecture (via ES8311 DAC)
    bool playAudio(const uint8_t* data, size_t length);
    bool isPlaying() { return playing; }
    void stopPlaying();

    // Controle du volume (0-100, defaut 50)
    void setVolume(int vol) { volume = constrain(vol, 0, 100); }
    int getVolume() { return volume; }

    // Test tonalité (pour debug speaker)
    void playTestTone(int frequency, int durationMs);

    // Rejouer l'audio enregistré via le speaker
    void playRecordedAudio();

    // Accès aux données audio
    uint8_t* getRecordingBuffer() { return recordBuffer; }
    size_t getRecordingSize() { return recordSize; }

    // Niveau audio (pour visualisation)
    int getInputLevel();

    // Test micro GPIO2 (pour debug)
    void testMicGPIO2();

    // Scanner pins (pour debug)
    void scanPDMPins();
    void scanI2SPins();

private:
    bool recording;
    bool playing;
    bool initialized;

    uint8_t* recordBuffer;
    size_t recordSize;
    size_t bufferCapacity;

    unsigned long recordStartTime;

    int volume;  // Volume 0-100, defaut 50
};

extern AudioManager audioManager;
extern I2SClass i2s_audio;  // Instance I2S globale

#endif
