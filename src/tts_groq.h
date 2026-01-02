#pragma once
#include <Arduino.h>

// Fonction pour récupérer un buffer WAV depuis Groq TTS
// Nécessite la clé API Groq et l'URL du service TTS
// Retourne true si succès, false sinon
// Le buffer alloué doit être libéré avec free()

bool getTTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize);

// Rate limit info (rempli si erreur 429)
extern bool ttsRateLimitHit;           // true si rate limit atteint
extern String ttsRateLimitRetryTime;   // Ex: "1h34m24s"
extern int ttsRateLimitUsed;           // Tokens utilises
extern int ttsRateLimitTotal;          // Limite totale

// Reset rate limit info
void resetTTSRateLimitInfo();
