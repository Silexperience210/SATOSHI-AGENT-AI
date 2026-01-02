#pragma once
#include <Arduino.h>

// OpenAI TTS comme alternative a Groq TTS
// Utilise le modele tts-1 avec voix alloy (rapide et naturelle)
// Retourne true si succes, false sinon
// Le buffer alloue doit etre libere avec free()

bool getOpenAITTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize);
