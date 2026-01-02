#pragma once
#include <Arduino.h>

// Google Cloud TTS via Vertex AI
// Utilise le modele Wavenet-fr (voix francaise naturelle)
// Retourne true si succes, false sinon
// Le buffer alloue doit etre libere avec free()

bool getGoogleTTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize);
