// audio.cpp - Gestion audio pour Freenove ESP32-S3 2.8" (FNK0104)
// Utilise ES8311 codec via I2S avec bibliothèque ESP_I2S
// Driver ES8311 officiel Freenove (API ESP-IDF)

#include "audio.h"
#include "freenove_es8311.h"  // Driver Freenove avec es8311_codec_init()
#include "rgb_led.h"          // LED RGB pour pulse audio
#include <Wire.h>

// Instance globale I2S
I2SClass i2s_audio;

// Instance globale AudioManager
AudioManager audioManager;

AudioManager::AudioManager() {
    recording = false;
    playing = false;
    initialized = false;
    recordBuffer = nullptr;
    recordSize = 0;
    bufferCapacity = 0;
    recordStartTime = 0;
    volume = 50;  // Volume par defaut 50%
}

bool AudioManager::begin() {
    Serial.println("Initialisation Audio (Freenove FNK0104)...");

    // Allouer buffer en PSRAM (5 secondes @ 16kHz, 16-bit mono)
    bufferCapacity = AUDIO_SAMPLE_RATE * 2 * 5;  // 160000 bytes
    if (psramFound()) {
        recordBuffer = (uint8_t*)ps_malloc(bufferCapacity);
        Serial.printf("Buffer PSRAM: %d bytes\n", bufferCapacity);
    } else {
        bufferCapacity = 32000;  // Fallback: 1 seconde
        recordBuffer = (uint8_t*)malloc(bufferCapacity);
        Serial.printf("Buffer RAM: %d bytes\n", bufferCapacity);
    }

    if (!recordBuffer) {
        Serial.println("ERREUR: Impossible d'allouer le buffer audio!");
        return false;
    }

    // Activer l'amplificateur (active LOW)
    pinMode(PIN_AUDIO_EN, OUTPUT);
    digitalWrite(PIN_AUDIO_EN, LOW);
    Serial.printf("Amplificateur GPIO%d = LOW (active)\n", PIN_AUDIO_EN);

    // Initialiser I2C pour ES8311 via Wire (I2C_NUM_0)
    // Le driver Freenove utilise l'API ESP-IDF qui requiert I2C_NUM_0
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);
    Serial.printf("I2C (Wire): SDA=%d, SCL=%d\n", PIN_TOUCH_SDA, PIN_TOUCH_SCL);

    // Configurer les pins I2S
    // setPins(bclk, ws, dout, din, mclk)
    i2s_audio.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
    Serial.printf("I2S pins: MCLK=%d, BCLK=%d, WS=%d, DOUT=%d, DIN=%d\n",
                  PIN_I2S_MCLK, PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN);

    // Initialiser I2S en mode standard, mono, 16kHz, 16-bit
    // I2S_STD_SLOT_LEFT pour lire depuis le microphone
    if (!i2s_audio.begin(I2S_MODE_STD, AUDIO_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                         I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
        Serial.println("ERREUR: Impossible d'initialiser I2S!");
        return false;
    }
    Serial.println("I2S initialise (16kHz, 16-bit, mono)");

    // Initialiser le codec ES8311 via le driver Freenove officiel
    // Utilise l'API ESP-IDF i2c_master_write_to_device sur I2C_NUM_0
    esp_err_t ret = es8311_codec_init();
    if (ret != ESP_OK) {
        Serial.printf("ERREUR: ES8311 init echoue (err=%d)\n", ret);
        return false;
    }
    Serial.println("ES8311 initialise (driver Freenove)");

    // Initialiser la LED RGB pour pulse audio
    rgbLed.begin();

    initialized = true;
    Serial.println("Audio initialise avec succes!");
    return true;
}

void AudioManager::end() {
    if (recordBuffer) {
        free(recordBuffer);
        recordBuffer = nullptr;
    }
    i2s_audio.end();
    initialized = false;
}

bool AudioManager::startRecording() {
    if (recording || !initialized) return false;

    // Vider le buffer I2S avant de commencer (eliminer les residus)
    uint8_t flushBuffer[1024];
    size_t flushed = 0;
    unsigned long flushStart = millis();
    while (millis() - flushStart < 100) {
        size_t n = i2s_audio.readBytes((char*)flushBuffer, sizeof(flushBuffer));
        if (n > 0) flushed += n;
        else break;
    }
    if (flushed > 0) {
        Serial.printf("Buffer I2S vide: %d bytes\n", flushed);
    }

    recordSize = 0;
    recordStartTime = millis();
    recording = true;

    Serial.println("\n========== ENREGISTREMENT (5 sec) ==========");
    Serial.println("Parlez dans le microphone!");

    // Utiliser recordWAV de ESP_I2S - plus simple et fiable
    size_t wav_size = 0;
    uint8_t* wav_buffer = i2s_audio.recordWAV(5, &wav_size);

    if (wav_buffer && wav_size > 44) {
        // Copier les données PCM (sans header WAV de 44 bytes)
        size_t pcm_size = wav_size - 44;
        if (pcm_size > bufferCapacity) {
            pcm_size = bufferCapacity;
        }
        memcpy(recordBuffer, wav_buffer + 44, pcm_size);
        recordSize = pcm_size;

        // Analyser le contenu
        int16_t* samples = (int16_t*)recordBuffer;
        int sampleCount = recordSize / 2;
        int32_t maxSample = -32768;
        int32_t minSample = 32767;
        int zeroCount = 0;

        for (int i = 0; i < sampleCount; i++) {
            if (samples[i] > maxSample) maxSample = samples[i];
            if (samples[i] < minSample) minSample = samples[i];
            if (samples[i] == 0) zeroCount++;
        }

        int range = maxSample - minSample;
        int zeroPercent = sampleCount > 0 ? (zeroCount * 100 / sampleCount) : 0;

        Serial.printf("Bytes: %d, Samples: %d\n", recordSize, sampleCount);
        Serial.printf("Amplitude: [%d, %d] range=%d\n", (int)minSample, (int)maxSample, range);
        Serial.printf("Zeros: %d%%\n", zeroPercent);

        // Afficher quelques samples
        Serial.print("Premiers samples: ");
        for (int i = 0; i < min(16, sampleCount); i++) {
            Serial.printf("%d ", samples[i]);
        }
        Serial.println();

        if (range < 100) {
            Serial.println("!!! ATTENTION: Signal tres faible !!!");
        } else {
            Serial.println("*** Enregistrement OK! ***");
        }

        free(wav_buffer);
    } else {
        Serial.println("ERREUR: recordWAV a echoue!");
        if (wav_buffer) free(wav_buffer);
    }

    recording = false;
    Serial.println("==============================================\n");
    return recordSize > 0;
}

bool AudioManager::startRecordingWithVAD(int maxDurationMs, int silenceMs) {
    if (recording || !initialized) return false;

    // Vider le buffer I2S avant de commencer
    uint8_t flushBuffer[1024];
    size_t flushed = 0;
    unsigned long flushStart = millis();
    while (millis() - flushStart < 100) {
        size_t n = i2s_audio.readBytes((char*)flushBuffer, sizeof(flushBuffer));
        if (n > 0) flushed += n;
        else break;
    }
    if (flushed > 0) {
        Serial.printf("Buffer I2S vide: %d bytes\n", flushed);
    }

    recordSize = 0;
    recordStartTime = millis();
    recording = true;

    Serial.println("\n========== ENREGISTREMENT VAD ==========");
    Serial.printf("Max: %dms, Silence: %dms\n", maxDurationMs, silenceMs);
    Serial.println("Parlez maintenant...");

    // Parametres VAD
    const int CHUNK_SIZE = 512;  // Samples par chunk
    const int ENERGY_THRESHOLD = 400;  // Seuil d'energie pour parole
    const int SPEECH_FRAMES_REQUIRED = 2;  // Frames pour confirmer debut parole
    const int silenceFramesRequired = (silenceMs * AUDIO_SAMPLE_RATE) / (CHUNK_SIZE * 1000);

    int16_t samples[CHUNK_SIZE];
    int speechFrameCount = 0;
    int silenceFrameCount = 0;
    bool speechStarted = false;
    unsigned long speechStartTime = 0;

    while (recording && (millis() - recordStartTime) < (unsigned long)maxDurationMs) {
        // Lire un chunk
        size_t bytesRead = i2s_audio.readBytes((char*)samples, sizeof(samples));
        if (bytesRead == 0) {
            delay(1);
            continue;
        }

        size_t sampleCount = bytesRead / sizeof(int16_t);

        // Calculer l'energie RMS
        int64_t sum = 0;
        for (size_t i = 0; i < sampleCount; i++) {
            int32_t s = samples[i];
            sum += s * s;
        }
        int energy = (int)sqrt((double)(sum / sampleCount));

        bool hasSpeech = (energy > ENERGY_THRESHOLD);

        if (hasSpeech) {
            speechFrameCount++;
            silenceFrameCount = 0;

            if (!speechStarted && speechFrameCount >= SPEECH_FRAMES_REQUIRED) {
                speechStarted = true;
                speechStartTime = millis();
                Serial.println("Parole detectee...");
            }
        } else {
            speechFrameCount = 0;
            if (speechStarted) {
                silenceFrameCount++;
            }
        }

        // Stocker l'audio si on a detecte de la parole
        if (speechStarted || speechFrameCount > 0) {
            if (recordSize + bytesRead < bufferCapacity) {
                memcpy(recordBuffer + recordSize, samples, bytesRead);
                recordSize += bytesRead;
            }
        }

        // Fin de parole detectee (silence apres parole)
        if (speechStarted && silenceFrameCount >= silenceFramesRequired) {
            unsigned long duration = millis() - speechStartTime;
            Serial.printf("Fin parole detectee apres %lu ms\n", duration);
            break;
        }

        yield();
    }

    recording = false;

    // Analyser l'enregistrement
    if (recordSize > 0) {
        int16_t* audioSamples = (int16_t*)recordBuffer;
        int sampleCount = recordSize / 2;
        int32_t maxSample = -32768, minSample = 32767;

        for (int i = 0; i < sampleCount; i++) {
            if (audioSamples[i] > maxSample) maxSample = audioSamples[i];
            if (audioSamples[i] < minSample) minSample = audioSamples[i];
        }

        int range = maxSample - minSample;
        int durationMs = (recordSize / 2) * 1000 / AUDIO_SAMPLE_RATE;
        Serial.printf("Enregistre: %d bytes, %d ms, range=%d\n", recordSize, durationMs, range);

        if (range < 100) {
            Serial.println("!!! Signal tres faible !!!");
        } else {
            Serial.println("*** Enregistrement VAD OK! ***");
        }
    } else {
        Serial.println("Aucune parole detectee");
    }

    Serial.println("=========================================\n");
    return recordSize > 1000;  // Au moins ~60ms d'audio
}

void AudioManager::stopRecording() {
    recording = false;
}

int AudioManager::getRecordingDuration() {
    if (recording) {
        return millis() - recordStartTime;
    }
    return 0;
}

bool AudioManager::playAudio(const uint8_t* data, size_t length) {
    if (playing || !initialized || !data || length == 0) return false;

    playing = true;
    Serial.printf("Lecture audio: %d bytes, volume=%d%%\n", length, volume);

    // Vérifier si c'est un fichier WAV (commence par "RIFF")
    if (length > 44 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F') {
        // C'est un WAV - extraire les infos du header
        uint16_t audioFormat = data[20] | (data[21] << 8);
        uint16_t numChannels = data[22] | (data[23] << 8);
        uint32_t sampleRate = data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24);
        uint16_t bitsPerSample = data[34] | (data[35] << 8);

        Serial.printf("WAV: %dHz, %d-bit, %d canaux, format=%d\n",
                      sampleRate, bitsPerSample, numChannels, audioFormat);

        // Si volume 100%, on force le chemin avec LED pour le pulse orange
        // playWAV ne permet pas le pulse LED, donc on passe par notre boucle
        if (false) {  // Desactive playWAV pour avoir le pulse LED
            i2s_audio.playWAV((uint8_t*)data, length);
        } else {
            // Volume reduit - reconfigurer I2S au bon sample rate puis ecrire par chunks
            // playWAV ne peut pas utiliser les donnees PSRAM directement
            // On reconfigure I2S puis on ecrit les samples avec volume ajuste

            Serial.printf("Lecture avec volume %d%% (sample rate %dHz)...\n", volume, sampleRate);

            // Reconfigurer I2S au sample rate du WAV
            i2s_audio.end();
            delay(10);
            i2s_audio.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
            i2s_audio.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT,
                           I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);

            // Ecrire les samples PCM avec volume par chunks
            const uint8_t* pcmData = data + 44;
            size_t pcmSize = length - 44;
            size_t offset = 0;
            const size_t chunkSize = 2048;  // 1024 samples * 2 bytes
            int16_t tempBuffer[1024];

            while (offset < pcmSize && playing) {
                size_t toProcess = min(chunkSize, pcmSize - offset);
                size_t numSamples = toProcess / 2;

                // Appliquer volume aux samples et calculer amplitude pour LED
                const int16_t* srcSamples = (const int16_t*)(pcmData + offset);
                int32_t maxAmp = 0;
                for (size_t i = 0; i < numSamples; i++) {
                    int32_t sample = srcSamples[i];
                    sample = (sample * volume) / 100;
                    if (sample > 32767) sample = 32767;
                    if (sample < -32768) sample = -32768;
                    tempBuffer[i] = (int16_t)sample;

                    // Calculer amplitude max pour LED
                    int32_t absVal = abs(sample);
                    if (absVal > maxAmp) maxAmp = absVal;
                }

                // Pulse LED orange avec amplitude (0-255)
                uint8_t ledBrightness = (maxAmp * 255) / 32768;
                rgbLed.pulseOrange(ledBrightness);

                i2s_audio.write((uint8_t*)tempBuffer, numSamples * 2);
                offset += toProcess;
            }

            // Eteindre LED apres lecture
            rgbLed.off();

            Serial.printf("Lecture terminee: %d bytes joues\n", offset);

            // Remettre I2S au sample rate original (16kHz pour le micro)
            i2s_audio.end();
            delay(10);
            i2s_audio.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
            i2s_audio.begin(I2S_MODE_STD, AUDIO_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                           I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
        }
        Serial.println("Lecture WAV terminee");
    } else {
        // Données PCM brutes - écrire directement avec volume
        Serial.println("Lecture PCM brut...");
        size_t offset = 0;
        const size_t chunkSize = 1024;
        int16_t tempBuffer[512];

        while (offset < length && playing) {
            size_t toProcess = min(chunkSize, length - offset);
            size_t numSamples = toProcess / 2;

            // Appliquer volume et calculer amplitude pour LED
            int16_t* srcSamples = (int16_t*)(data + offset);
            int32_t maxAmp = 0;
            for (size_t i = 0; i < numSamples; i++) {
                int32_t sample = srcSamples[i];
                sample = (sample * volume) / 100;
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                tempBuffer[i] = (int16_t)sample;

                // Calculer amplitude max pour LED
                int32_t absVal = abs(sample);
                if (absVal > maxAmp) maxAmp = absVal;
            }

            // Pulse LED orange avec amplitude
            uint8_t ledBrightness = (maxAmp * 255) / 32768;
            rgbLed.pulseOrange(ledBrightness);

            size_t bytesWritten = i2s_audio.write((uint8_t*)tempBuffer, numSamples * 2);
            offset += bytesWritten;
            yield();
        }

        // Eteindre LED apres lecture
        rgbLed.off();
        Serial.printf("PCM joue: %d bytes\n", offset);
    }

    playing = false;
    return true;
}

void AudioManager::stopPlaying() {
    playing = false;
}

void AudioManager::playTestTone(int frequency, int durationMs) {
    if (!initialized) return;

    Serial.printf("Test tone: %dHz pendant %dms (volume %d%%)\n", frequency, durationMs, volume);

    const int sampleRate = AUDIO_SAMPLE_RATE;
    const int samples = (sampleRate * durationMs) / 1000;
    // Amplitude reduite par volume (base 8000 * volume%)
    const int amplitude = (8000 * volume) / 100;

    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    if (!buffer) return;

    // Générer une sinusoïde avec volume applique
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sampleRate;
        buffer[i] = (int16_t)(amplitude * sin(2.0 * PI * frequency * t));
    }

    playing = true;
    i2s_audio.write((uint8_t*)buffer, samples * sizeof(int16_t));
    playing = false;

    free(buffer);
}

void AudioManager::playRecordedAudio() {
    if (!initialized) {
        Serial.println("Audio non initialise!");
        return;
    }

    if (recordSize == 0) {
        Serial.println("Aucun enregistrement! Utilisez /record d'abord.");
        return;
    }

    Serial.println("========== LECTURE ENREGISTREMENT ==========");
    Serial.printf("Taille: %d bytes (%d ms)\n", recordSize, (int)((recordSize / 2) * 1000 / AUDIO_SAMPLE_RATE));

    playing = true;

    // Écrire directement les données PCM
    size_t written = i2s_audio.write(recordBuffer, recordSize);
    Serial.printf("Ecrit: %d bytes\n", written);

    playing = false;
    Serial.println("========== FIN LECTURE ==========\n");
}

int AudioManager::getInputLevel() {
    // Pour monitoring en temps réel - non implémenté pour l'instant
    return 0;
}

void AudioManager::testMicGPIO2() {
    Serial.println("========== TEST MICRO GPIO2 ==========");
    Serial.println("Test ADC sur GPIO2...");

    pinMode(2, INPUT);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    int32_t minVal = 4095, maxVal = 0;
    int64_t sum = 0;

    for (int i = 0; i < 1000; i++) {
        int val = analogRead(2);
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
        sum += val;
        delayMicroseconds(100);
    }

    int avg = sum / 1000;
    int range = maxVal - minVal;

    Serial.printf("GPIO2 ADC: min=%d, max=%d, avg=%d, range=%d\n",
                  (int)minVal, (int)maxVal, avg, range);

    if (range > 50) {
        Serial.println("Signal detecte sur GPIO2!");
    } else {
        Serial.println("Pas de signal sur GPIO2");
    }

    Serial.println("======================================\n");
}

void AudioManager::scanPDMPins() {
    Serial.println("Scan PDM non necessaire - utilise ES8311 I2S");
}

void AudioManager::scanI2SPins() {
    Serial.printf("I2S configure: MCLK=%d, BCLK=%d, WS=%d, DOUT=%d, DIN=%d\n",
                  PIN_I2S_MCLK, PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN);
}
