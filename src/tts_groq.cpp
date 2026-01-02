#include "tts_groq.h"
#include "config.h"
#include <WiFiClientSecure.h>

// Variables globales pour rate limit
bool ttsRateLimitHit = false;
String ttsRateLimitRetryTime = "";
int ttsRateLimitUsed = 0;
int ttsRateLimitTotal = 0;

void resetTTSRateLimitInfo() {
    ttsRateLimitHit = false;
    ttsRateLimitRetryTime = "";
    ttsRateLimitUsed = 0;
    ttsRateLimitTotal = 0;
}

// Parse le message d'erreur rate limit pour extraire le timing
// Format: "...Please try again in 1h34m24s..."
void parseRateLimitError(const String& errorBody) {
    ttsRateLimitHit = true;

    // Chercher "try again in "
    int retryIdx = errorBody.indexOf("try again in ");
    if (retryIdx > 0) {
        int startIdx = retryIdx + 13;  // apres "try again in "
        int endIdx = errorBody.indexOf(".", startIdx);
        if (endIdx < 0) endIdx = errorBody.indexOf("\"", startIdx);
        if (endIdx > startIdx) {
            ttsRateLimitRetryTime = errorBody.substring(startIdx, endIdx);
            ttsRateLimitRetryTime.trim();
        }
    }

    // Chercher "Limit " et "Used "
    int limitIdx = errorBody.indexOf("Limit ");
    if (limitIdx > 0) {
        int startIdx = limitIdx + 6;
        int endIdx = startIdx;
        while (endIdx < errorBody.length() && isDigit(errorBody.charAt(endIdx))) endIdx++;
        if (endIdx > startIdx) {
            ttsRateLimitTotal = errorBody.substring(startIdx, endIdx).toInt();
        }
    }

    int usedIdx = errorBody.indexOf("Used ");
    if (usedIdx > 0) {
        int startIdx = usedIdx + 5;
        int endIdx = startIdx;
        while (endIdx < errorBody.length() && isDigit(errorBody.charAt(endIdx))) endIdx++;
        if (endIdx > startIdx) {
            ttsRateLimitUsed = errorBody.substring(startIdx, endIdx).toInt();
        }
    }

    Serial.printf("Rate limit parse: retry=%s, used=%d/%d\n",
                  ttsRateLimitRetryTime.c_str(), ttsRateLimitUsed, ttsRateLimitTotal);
}

#define GROQ_TTS_HOST "api.groq.com"
#define GROQ_TTS_PORT 443
#define GROQ_TTS_PATH "/openai/v1/audio/speech"

// Echappe les caracteres speciaux pour JSON
String escapeJsonString(const char* text) {
    String result = "";
    while (*text) {
        char c = *text++;
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += " "; break;  // Remplace newline par espace
            case '\r': break;  // Ignore CR
            case '\t': result += " "; break;  // Remplace tab par espace
            default:
                if (c >= 32 && c < 127) {
                    result += c;
                }
                // Ignore les autres caracteres de controle
                break;
        }
    }
    return result;
}

// Utilise WiFiClientSecure pour HTTPS
bool getTTSWavBuffer(const char* text, uint8_t** outBuffer, size_t* outSize) {
    WiFiClientSecure client;
    client.setInsecure(); // Pour test, à sécuriser en prod
    if (!client.connect(GROQ_TTS_HOST, GROQ_TTS_PORT)) {
        Serial.println("Erreur connexion Groq TTS");
        return false;
    }

    // Groq TTS - modele Orpheus (conditions acceptees)
    String escapedText = escapeJsonString(text);
    // Voix disponibles: autumn, diana, hannah, austin, daniel, troy
    String payload = String("{\"model\":\"canopylabs/orpheus-v1-english\",\"input\":\"") + escapedText + "\",\"voice\":\"daniel\",\"response_format\":\"wav\"}";

    String headers = String("POST ") + GROQ_TTS_PATH + " HTTP/1.1\r\n" +
        "Host: " + GROQ_TTS_HOST + "\r\n" +
        "Authorization: Bearer " + String(configManager.config.groq_key) + "\r\n" +
        "Content-Type: application/json\r\n" +
        "Accept: audio/wav\r\n" +
        "Content-Length: " + payload.length() + "\r\n\r\n";

    client.print(headers);
    client.print(payload);

    // Attendre la réponse
    while (client.connected() && !client.available()) delay(10);
    String responseHeaders = "";
    String statusLine = "";
    bool firstLine = true;
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (firstLine) {
            statusLine = line;
            firstLine = false;
            Serial.print("TTS HTTP Status: ");
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

    // Chercher Content-Length ou Transfer-Encoding
    int contentLength = -1;
    bool isChunked = false;
    int idx = responseHeaders.indexOf("Content-Length: ");
    if (idx >= 0) {
        int endIdx = responseHeaders.indexOf("\r", idx);
        if (endIdx < 0) endIdx = responseHeaders.indexOf("\n", idx);
        contentLength = responseHeaders.substring(idx + 16, endIdx).toInt();
    }
    if (responseHeaders.indexOf("Transfer-Encoding: chunked") >= 0 ||
        responseHeaders.indexOf("transfer-encoding: chunked") >= 0) {
        isChunked = true;
    }

    Serial.printf("TTS Content-Length: %d, Chunked: %s\n", contentLength, isChunked ? "yes" : "no");

    // Si erreur HTTP, lire et afficher le body pour debug
    if (httpCode != 200) {
        Serial.print("Erreur TTS HTTP ");
        Serial.println(httpCode);
        String errorBody = "";
        unsigned long timeout = millis() + 5000;
        while (client.available() && errorBody.length() < 2000 && millis() < timeout) {
            errorBody += (char)client.read();
        }
        if (errorBody.length() > 0) {
            Serial.print("Erreur TTS body: ");
            Serial.println(errorBody);

            // Parser le rate limit si erreur 429
            if (httpCode == 429) {
                parseRateLimitError(errorBody);
            }
        }
        return false;
    }

    // Lire le body (chunked ou content-length)
    if (isChunked) {
        // Transfer-Encoding: chunked - lire par morceaux
        // Format: <hex size>\r\n<data>\r\n ... 0\r\n\r\n
        Serial.println("Lecture TTS chunked...");

        // Buffer dynamique (max 1MB pour audio long)
        size_t maxSize = 1000000;
        *outBuffer = (uint8_t*)ps_malloc(maxSize);
        if (!*outBuffer) {
            *outBuffer = (uint8_t*)malloc(maxSize);
        }
        if (!*outBuffer) {
            Serial.println("Erreur malloc TTS buffer chunked");
            return false;
        }

        *outSize = 0;
        unsigned long timeout = millis() + 30000;  // 30s timeout

        while (client.connected() && millis() < timeout) {
            // Lire la taille du chunk (hex)
            String chunkSizeLine = "";
            while (client.connected() && millis() < timeout) {
                if (client.available()) {
                    char c = client.read();
                    if (c == '\n') break;
                    if (c != '\r') chunkSizeLine += c;
                } else {
                    delay(1);
                }
            }

            // Parser la taille hex
            int chunkSize = (int)strtol(chunkSizeLine.c_str(), NULL, 16);
            if (chunkSize == 0) {
                // Fin des chunks
                Serial.printf("TTS chunked complete: %d bytes\n", *outSize);
                break;
            }

            // Lire les donnees du chunk
            size_t chunkRead = 0;
            while (chunkRead < chunkSize && client.connected() && millis() < timeout) {
                if (client.available()) {
                    int n = client.read(*outBuffer + *outSize, min((size_t)(chunkSize - chunkRead), maxSize - *outSize));
                    if (n > 0) {
                        chunkRead += n;
                        *outSize += n;
                    }
                } else {
                    delay(1);
                }

                if (*outSize >= maxSize) {
                    Serial.println("Buffer TTS plein!");
                    break;
                }
            }

            // Lire le \r\n apres le chunk
            while (client.connected() && client.available() < 2 && millis() < timeout) delay(1);
            if (client.available() >= 2) {
                client.read(); // \r
                client.read(); // \n
            }
        }

        if (*outSize == 0) {
            Serial.println("Erreur: aucune donnee TTS recue");
            free(*outBuffer);
            *outBuffer = nullptr;
            return false;
        }

        return true;

    } else if (contentLength > 0) {
        // Content-Length standard
        *outBuffer = (uint8_t*)malloc(contentLength);
        if (!*outBuffer) {
            Serial.println("Erreur malloc TTS buffer");
            return false;
        }
        *outSize = contentLength;
        size_t received = 0;
        unsigned long timeout = millis() + 30000;
        while (received < (size_t)contentLength && client.connected() && millis() < timeout) {
            int n = client.read(*outBuffer + received, contentLength - received);
            if (n > 0) received += n;
            else delay(1);
        }
        if (received != (size_t)contentLength) {
            Serial.println("Erreur: taille audio TTS incomplete");
            free(*outBuffer);
            *outBuffer = nullptr;
            *outSize = 0;
            return false;
        }
        return true;
    } else {
        Serial.println("Erreur: ni Content-Length ni chunked");
        return false;
    }
}
