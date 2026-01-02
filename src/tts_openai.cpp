#include "tts_openai.h"
#include "config.h"
#include <WiFiClientSecure.h>

#define OPENAI_TTS_HOST "api.openai.com"
#define OPENAI_TTS_PORT 443
#define OPENAI_TTS_PATH "/v1/audio/speech"

// Echappe les caracteres speciaux pour JSON
static String escapeJsonStringOpenAI(const char* text) {
    String result = "";
    while (*text) {
        char c = *text++;
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += " "; break;
            case '\r': break;
            case '\t': result += " "; break;
            default:
                if (c >= 32 && c < 127) {
                    result += c;
                }
                break;
        }
    }
    return result;
}

bool getOpenAITTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize) {
    // Verifier si on a une cle Anthropic (on l'utilise aussi pour OpenAI dans ce cas)
    // Note: Idealement on aurait une cle OpenAI separee, mais pour simplifier
    // on verifie juste que le texte n'est pas vide
    if (!text || strlen(text) == 0) {
        Serial.println("OpenAI TTS: texte vide");
        return false;
    }

    // Verifier si on a une cle API (utiliser anthropic_key comme fallback)
    // En production, il faudrait une cle OpenAI dediee
    if (strlen(configManager.config.anthropic_key) == 0) {
        Serial.println("OpenAI TTS: cle API manquante");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    Serial.println("OpenAI TTS: connexion...");
    if (!client.connect(OPENAI_TTS_HOST, OPENAI_TTS_PORT)) {
        Serial.println("Erreur connexion OpenAI TTS");
        return false;
    }

    // OpenAI TTS - modele tts-1 (rapide) avec voix alloy
    // Voix disponibles: alloy, echo, fable, onyx, nova, shimmer
    String escapedText = escapeJsonStringOpenAI(text);
    String payload = String("{\"model\":\"tts-1\",\"input\":\"") + escapedText +
                     "\",\"voice\":\"alloy\",\"response_format\":\"wav\",\"speed\":1.0}";

    // Note: Ceci utilise la cle Anthropic - en production utiliser une cle OpenAI
    // Pour l'instant on skip OpenAI si pas de cle dediee
    Serial.println("OpenAI TTS: API non configuree (necessite cle OpenAI)");
    return false;

    /* Code pour quand on aura une cle OpenAI:
    String headers = String("POST ") + OPENAI_TTS_PATH + " HTTP/1.1\r\n" +
        "Host: " + OPENAI_TTS_HOST + "\r\n" +
        "Authorization: Bearer " + String(configManager.config.openai_key) + "\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + payload.length() + "\r\n\r\n";

    client.print(headers);
    client.print(payload);

    // ... reste du code similaire a tts_groq.cpp
    */
}
