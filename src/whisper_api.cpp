// whisper_api.cpp - Implementation API Groq Whisper
#include "whisper_api.h"
#include <ArduinoJson.h>

WhisperAPI whisperAPI;

WhisperAPI::WhisperAPI() {
    client = nullptr;
}

void WhisperAPI::begin() {
    client = new WiFiClientSecure();
    client->setInsecure();
    Serial.println("Groq Whisper API initialisee");
}

size_t WhisperAPI::createWavHeader(uint8_t* header, size_t dataSize) {
    // Parametres WAV
    uint32_t sampleRate = 16000;
    uint16_t numChannels = 1;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    uint16_t blockAlign = numChannels * bitsPerSample / 8;
    uint32_t chunkSize = 36 + dataSize;

    // RIFF header
    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    header[4] = chunkSize & 0xFF;
    header[5] = (chunkSize >> 8) & 0xFF;
    header[6] = (chunkSize >> 16) & 0xFF;
    header[7] = (chunkSize >> 24) & 0xFF;
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

    // fmt chunk
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;  // Subchunk1Size
    header[20] = 1; header[21] = 0;  // AudioFormat (PCM)
    header[22] = numChannels & 0xFF; header[23] = (numChannels >> 8) & 0xFF;
    header[24] = sampleRate & 0xFF;
    header[25] = (sampleRate >> 8) & 0xFF;
    header[26] = (sampleRate >> 16) & 0xFF;
    header[27] = (sampleRate >> 24) & 0xFF;
    header[28] = byteRate & 0xFF;
    header[29] = (byteRate >> 8) & 0xFF;
    header[30] = (byteRate >> 16) & 0xFF;
    header[31] = (byteRate >> 24) & 0xFF;
    header[32] = blockAlign & 0xFF; header[33] = (blockAlign >> 8) & 0xFF;
    header[34] = bitsPerSample & 0xFF; header[35] = (bitsPerSample >> 8) & 0xFF;

    // data chunk
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    header[40] = dataSize & 0xFF;
    header[41] = (dataSize >> 8) & 0xFF;
    header[42] = (dataSize >> 16) & 0xFF;
    header[43] = (dataSize >> 24) & 0xFF;

    return 44;  // Taille du header WAV
}

bool WhisperAPI::transcribe(const uint8_t* audioData, size_t audioSize, String& transcription) {
    return transcribeWithPrompt(audioData, audioSize, transcription, nullptr);
}

bool WhisperAPI::transcribeWithPrompt(const uint8_t* audioData, size_t audioSize, String& transcription, const char* prompt, const char* language) {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    if (strlen(configManager.config.groq_key) == 0) {
        lastError = "Cle Groq manquante";
        return false;
    }

    if (audioSize < 1000) {
        lastError = "Audio trop court";
        return false;
    }

    Serial.printf("Transcription Groq: %d bytes (lang=%s)\n", audioSize, language);

    // Creer le header WAV
    uint8_t wavHeader[44];
    createWavHeader(wavHeader, audioSize);

    // Generer un boundary unique
    String boundary = "----ESP32Boundary" + String(millis());

    // Construire le body multipart
    String bodyStart = "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    bodyStart += "Content-Type: audio/wav\r\n\r\n";

    String bodyMiddle = "\r\n--" + boundary + "\r\n";
    bodyMiddle += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    bodyMiddle += WHISPER_MODEL;
    bodyMiddle += "\r\n--" + boundary + "\r\n";
    bodyMiddle += "Content-Disposition: form-data; name=\"language\"\r\n\r\n";
    bodyMiddle += language;  // Utiliser la langue specifiee

    // Ajouter le prompt si fourni (aide Whisper a reconnaitre des mots specifiques)
    if (prompt && strlen(prompt) > 0) {
        bodyMiddle += "\r\n--" + boundary + "\r\n";
        bodyMiddle += "Content-Disposition: form-data; name=\"prompt\"\r\n\r\n";
        bodyMiddle += prompt;
    }

    bodyMiddle += "\r\n--" + boundary + "--\r\n";

    // Calculer la taille totale
    size_t totalSize = bodyStart.length() + 44 + audioSize + bodyMiddle.length();

    Serial.printf("Envoi a Groq: %d bytes total\n", totalSize);

    // Connexion
    Serial.println("Connexion a api.groq.com...");
    client->setTimeout(60);

    if (!client->connect("api.groq.com", 443)) {
        lastError = "Connexion echouee";
        Serial.println("ERREUR: Connexion a api.groq.com echouee!");
        return false;
    }
    Serial.println("Connecte a api.groq.com");

    // Envoyer les headers HTTP
    Serial.println("Envoi requete HTTP...");
    client->println("POST /openai/v1/audio/transcriptions HTTP/1.1");
    client->println("Host: api.groq.com");
    client->print("Authorization: Bearer ");
    client->println(configManager.config.groq_key);
    client->print("Content-Type: multipart/form-data; boundary=");
    client->println(boundary);
    client->print("Content-Length: ");
    client->println(totalSize);
    client->println("Connection: close");
    client->println();

    // Envoyer le body
    client->print(bodyStart);
    client->write(wavHeader, 44);

    // Envoyer l'audio par morceaux
    size_t offset = 0;
    size_t chunkSize = 1024;
    while (offset < audioSize) {
        size_t toSend = min(chunkSize, audioSize - offset);
        client->write(audioData + offset, toSend);
        offset += toSend;
        yield();
    }

    client->print(bodyMiddle);
    Serial.println("Requete envoyee, attente reponse...");

    // Attendre la reponse
    unsigned long timeout = millis() + 60000;
    while (client->connected() && !client->available()) {
        if (millis() > timeout) {
            lastError = "Timeout";
            client->stop();
            return false;
        }
        delay(10);
    }

    // Lire la reponse
    String response = "";
    bool headersDone = false;

    while (client->available()) {
        String line = client->readStringUntil('\n');
        if (!headersDone) {
            if (line == "\r") {
                headersDone = true;
            }
        } else {
            response += line;
        }
    }

    client->stop();

    Serial.println("Reponse Groq:");
    Serial.println(response);

    // Parser la reponse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON: " + String(error.c_str());
        return false;
    }

    if (doc["error"].is<JsonObject>()) {
        lastError = doc["error"]["message"].as<String>();
        return false;
    }

    if (doc["text"].is<String>()) {
        transcription = doc["text"].as<String>();
        Serial.println("Transcription: " + transcription);
        return true;
    }

    lastError = "Format de reponse invalide";
    return false;
}
