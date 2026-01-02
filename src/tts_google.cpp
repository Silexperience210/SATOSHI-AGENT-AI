#include "tts_google.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Google Cloud TTS API endpoint
#define GOOGLE_TTS_HOST "texttospeech.googleapis.com"
#define GOOGLE_TTS_PORT 443
#define GOOGLE_TTS_PATH "/v1/text:synthesize"

// Echappe les caracteres speciaux pour JSON
static String escapeJsonStringGoogle(const char* text) {
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
                if (c >= 32) {
                    result += c;
                }
                break;
        }
    }
    return result;
}

bool getGoogleTTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize) {
    if (!text || strlen(text) == 0) {
        Serial.println("Google TTS: texte vide");
        return false;
    }

    // Verifier si on a une cle API Google
    if (strlen(configManager.config.google_tts_key) == 0) {
        Serial.println("Google TTS: cle API manquante");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    Serial.println("Google TTS: connexion...");
    if (!client.connect(GOOGLE_TTS_HOST, GOOGLE_TTS_PORT)) {
        Serial.println("Erreur connexion Google TTS");
        return false;
    }

    // Construire le payload JSON
    String escapedText = escapeJsonStringGoogle(text);

    // Google Cloud TTS API format
    // Voix francaises: fr-FR-Wavenet-A (femme), fr-FR-Wavenet-B (homme)
    String payload = "{\"input\":{\"text\":\"" + escapedText + "\"},"
                     "\"voice\":{\"languageCode\":\"fr-FR\",\"name\":\"fr-FR-Wavenet-D\",\"ssmlGender\":\"MALE\"},"
                     "\"audioConfig\":{\"audioEncoding\":\"LINEAR16\",\"sampleRateHertz\":16000}}";

    // URL avec cle API
    String path = String(GOOGLE_TTS_PATH) + "?key=" + String(configManager.config.google_tts_key);

    String headers = String("POST ") + path + " HTTP/1.1\r\n" +
        "Host: " + GOOGLE_TTS_HOST + "\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + payload.length() + "\r\n\r\n";

    Serial.println("Google TTS: envoi requete...");
    client.print(headers);
    client.print(payload);

    // Attendre la reponse
    unsigned long timeout = millis() + 10000;
    while (client.connected() && !client.available() && millis() < timeout) {
        delay(10);
    }

    // Lire les headers de reponse
    String statusLine = "";
    String responseHeaders = "";
    bool firstLine = true;

    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (firstLine) {
            statusLine = line;
            firstLine = false;
            Serial.print("Google TTS HTTP Status: ");
            Serial.println(statusLine);
        }
        if (line == "\r" || line == "") break;
        responseHeaders += line + "\n";
    }

    // Verifier le code HTTP
    int httpCode = 0;
    if (statusLine.startsWith("HTTP/1.1 ")) {
        httpCode = statusLine.substring(9, 12).toInt();
    }

    if (httpCode != 200) {
        Serial.printf("Erreur Google TTS HTTP %d\n", httpCode);
        // Lire le body d'erreur
        String errorBody = "";
        timeout = millis() + 5000;
        while (client.available() && errorBody.length() < 2000 && millis() < timeout) {
            errorBody += (char)client.read();
        }
        if (errorBody.length() > 0) {
            Serial.print("Erreur body: ");
            Serial.println(errorBody);
        }
        return false;
    }

    // Chercher Content-Length
    int contentLength = -1;
    int idx = responseHeaders.indexOf("Content-Length: ");
    if (idx >= 0) {
        int endIdx = responseHeaders.indexOf("\r", idx);
        if (endIdx < 0) endIdx = responseHeaders.indexOf("\n", idx);
        contentLength = responseHeaders.substring(idx + 16, endIdx).toInt();
    }

    Serial.printf("Google TTS Content-Length: %d\n", contentLength);

    // Lire le body JSON
    String jsonResponse = "";
    timeout = millis() + 30000;

    if (contentLength > 0) {
        while (jsonResponse.length() < (size_t)contentLength && client.connected() && millis() < timeout) {
            if (client.available()) {
                jsonResponse += (char)client.read();
            } else {
                delay(1);
            }
        }
    } else {
        // Pas de Content-Length, lire jusqu'a fermeture
        while (client.connected() && millis() < timeout) {
            if (client.available()) {
                jsonResponse += (char)client.read();
            } else {
                delay(10);
            }
        }
    }

    Serial.printf("Google TTS reponse JSON: %d bytes\n", jsonResponse.length());

    // Parser le JSON pour extraire l'audio base64
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        Serial.print("Erreur JSON Google TTS: ");
        Serial.println(error.c_str());
        return false;
    }

    // L'audio est dans "audioContent" en base64
    const char* audioBase64 = doc["audioContent"];
    if (!audioBase64) {
        Serial.println("Google TTS: audioContent manquant");
        return false;
    }

    size_t base64Len = strlen(audioBase64);
    Serial.printf("Google TTS audio base64: %d chars\n", base64Len);

    // Decoder base64 -> PCM
    // Taille estimee: base64 est ~33% plus grand que les donnees binaires
    size_t estimatedSize = (base64Len * 3) / 4 + 10;

    uint8_t* pcmData = (uint8_t*)ps_malloc(estimatedSize);
    if (!pcmData) {
        pcmData = (uint8_t*)malloc(estimatedSize);
    }
    if (!pcmData) {
        Serial.println("Erreur malloc Google TTS buffer");
        return false;
    }

    // Decoder base64
    size_t decodedLen = 0;

    // Table de decodage base64
    static const int8_t base64_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    uint32_t buf = 0;
    int bits = 0;

    for (size_t i = 0; i < base64Len; i++) {
        int8_t val = base64_table[(uint8_t)audioBase64[i]];
        if (val == -1) continue;  // Skip invalid chars (newlines, etc)

        buf = (buf << 6) | val;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            pcmData[decodedLen++] = (buf >> bits) & 0xFF;
        }
    }

    Serial.printf("Google TTS decoded: %d bytes PCM\n", decodedLen);

    // Creer un header WAV
    size_t wavSize = 44 + decodedLen;
    *outBuffer = (uint8_t*)ps_malloc(wavSize);
    if (!*outBuffer) {
        *outBuffer = (uint8_t*)malloc(wavSize);
    }
    if (!*outBuffer) {
        free(pcmData);
        Serial.println("Erreur malloc WAV buffer");
        return false;
    }

    // WAV header
    uint8_t* wav = *outBuffer;
    memcpy(wav, "RIFF", 4);
    uint32_t fileSize = wavSize - 8;
    wav[4] = fileSize & 0xFF;
    wav[5] = (fileSize >> 8) & 0xFF;
    wav[6] = (fileSize >> 16) & 0xFF;
    wav[7] = (fileSize >> 24) & 0xFF;
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    wav[16] = 16; wav[17] = 0; wav[18] = 0; wav[19] = 0;  // Subchunk1Size
    wav[20] = 1; wav[21] = 0;  // AudioFormat (PCM)
    wav[22] = 1; wav[23] = 0;  // NumChannels (mono)
    uint32_t sampleRate = 16000;
    wav[24] = sampleRate & 0xFF;
    wav[25] = (sampleRate >> 8) & 0xFF;
    wav[26] = (sampleRate >> 16) & 0xFF;
    wav[27] = (sampleRate >> 24) & 0xFF;
    uint32_t byteRate = sampleRate * 2;  // 16-bit mono
    wav[28] = byteRate & 0xFF;
    wav[29] = (byteRate >> 8) & 0xFF;
    wav[30] = (byteRate >> 16) & 0xFF;
    wav[31] = (byteRate >> 24) & 0xFF;
    wav[32] = 2; wav[33] = 0;  // BlockAlign
    wav[34] = 16; wav[35] = 0;  // BitsPerSample
    memcpy(wav + 36, "data", 4);
    uint32_t dataSize = decodedLen;
    wav[40] = dataSize & 0xFF;
    wav[41] = (dataSize >> 8) & 0xFF;
    wav[42] = (dataSize >> 16) & 0xFF;
    wav[43] = (dataSize >> 24) & 0xFF;

    // Copier les donnees PCM
    memcpy(wav + 44, pcmData, decodedLen);
    free(pcmData);

    *outSize = wavSize;
    Serial.printf("Google TTS WAV: %d bytes\n", wavSize);

    return true;
}
