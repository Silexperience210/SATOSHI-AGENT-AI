// whisper_api.h - API Groq Whisper pour transcription audio
#ifndef WHISPER_API_H
#define WHISPER_API_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

// Groq API (compatible Whisper, gratuit avec premium)
#define GROQ_API_URL "https://api.groq.com/openai/v1/audio/transcriptions"
// Whisper large-v3-turbo: 8x plus rapide que large-v3, qualite similaire
#define WHISPER_MODEL "whisper-large-v3-turbo"

class WhisperAPI {
public:
    WhisperAPI();

    void begin();

    // Transcrire l'audio en texte (francais par defaut)
    bool transcribe(const uint8_t* audioData, size_t audioSize, String& transcription);

    // Transcrire avec un prompt hint et langue specifiee (pour wake word)
    bool transcribeWithPrompt(const uint8_t* audioData, size_t audioSize, String& transcription, const char* prompt, const char* language = "fr");

    String getLastError() { return lastError; }

private:
    WiFiClientSecure* client;
    String lastError;

    // Creer un fichier WAV en memoire
    size_t createWavHeader(uint8_t* header, size_t dataSize);
};

extern WhisperAPI whisperAPI;

#endif
