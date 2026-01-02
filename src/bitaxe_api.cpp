// bitaxe_api.cpp - Implémentation API Bitaxe
#include "bitaxe_api.h"
#include <WiFi.h>

BitaxeAPI bitaxeAPI;

BitaxeAPI::BitaxeAPI() {
    lastError = "";
}

bool BitaxeAPI::fetchMinerInfo(const char* ip, BitaxeMinerInfo& info) {
    memset(&info, 0, sizeof(info));
    strncpy(info.ip, ip, sizeof(info.ip) - 1);
    info.valid = false;
    info.isOnline = false;

    if (WiFi.status() != WL_CONNECTED) {
        lastError = "WiFi non connecté";
        return false;
    }

    HTTPClient http;
    String url = "http://" + String(ip) + "/api/system/info";

    http.begin(url);
    http.setTimeout(5000);  // 5 secondes timeout

    int httpCode = http.GET();

    if (httpCode != 200) {
        if (httpCode < 0) {
            lastError = "Mineur hors ligne";
        } else {
            lastError = "Erreur HTTP: " + String(httpCode);
        }
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    if (!parseSystemInfo(payload, info)) {
        return false;
    }

    info.valid = true;
    info.isOnline = true;
    info.lastUpdate = millis();

    return true;
}

bool BitaxeAPI::parseSystemInfo(const String& json, BitaxeMinerInfo& info) {
    // Parser JSON avec ArduinoJson
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        lastError = "Erreur JSON: " + String(error.c_str());
        Serial.println(lastError);
        return false;
    }

    // Infos générales
    if (doc.containsKey("hostname")) {
        strncpy(info.hostname, doc["hostname"] | "", sizeof(info.hostname) - 1);
    }
    if (doc.containsKey("version")) {
        strncpy(info.version, doc["version"] | "", sizeof(info.version) - 1);
    }
    if (doc.containsKey("ASICModel")) {
        strncpy(info.asicModel, doc["ASICModel"] | "", sizeof(info.asicModel) - 1);
    }

    // Performance - hashRate est en GH/s
    info.hashrate = doc["hashRate"] | 0.0f;

    // Power déjà en Watts depuis l'API Bitaxe
    info.power = doc["power"] | 0.0f;

    // Calculer l'efficacité (J/TH)
    if (info.hashrate > 0) {
        // hashrate en GH/s, power en W
        // Efficacité = W / (GH/s / 1000) = W * 1000 / GH/s = J/TH
        info.efficiency = (info.power * 1000.0f) / info.hashrate;
    }

    // Voltage en mV
    info.voltage = doc["voltage"] | 0.0f;

    // Fréquence en MHz
    info.frequency = doc["frequency"] | 0.0f;

    // Température
    info.tempChip = doc["temp"] | 0.0f;
    if (doc.containsKey("vrTemp")) {
        info.tempVrm = doc["vrTemp"] | 0.0f;
    }

    // Mining stats
    info.sharesAccepted = doc["sharesAccepted"] | 0;
    info.sharesRejected = doc["sharesRejected"] | 0;

    // Best difficulty
    if (doc.containsKey("bestDiff")) {
        String bestDiffStr = doc["bestDiff"].as<String>();
        strncpy(info.bestDiffStr, bestDiffStr.c_str(), sizeof(info.bestDiffStr) - 1);
        // Essayer de parser la valeur numérique
        info.bestDiff = bestDiffStr.toInt();
    }

    // Pool info
    if (doc.containsKey("stratumURL")) {
        strncpy(info.poolUrl, doc["stratumURL"] | "", sizeof(info.poolUrl) - 1);
    }
    if (doc.containsKey("stratumUser")) {
        strncpy(info.poolUser, doc["stratumUser"] | "", sizeof(info.poolUser) - 1);
    }

    // Uptime en secondes
    info.uptimeSeconds = doc["uptimeSeconds"] | 0;

    Serial.printf("Bitaxe %s: %.2f GH/s, %.1fW, %.1f°C\n",
                  info.hostname, info.hashrate, info.power, info.tempChip);

    return true;
}

MiningStats BitaxeAPI::getAggregatedStats() {
    MiningStats stats;
    memset(&stats, 0, sizeof(stats));
    // Cette fonction sera appelée par MiningManager qui possède la liste des mineurs
    return stats;
}

String BitaxeAPI::formatHashrate(float ghps) {
    char buf[32];
    if (ghps >= 1000) {
        snprintf(buf, sizeof(buf), "%.2f TH/s", ghps / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f GH/s", ghps);
    }
    return String(buf);
}

String BitaxeAPI::formatUptime(uint32_t seconds) {
    char buf[32];
    uint32_t days = seconds / 86400;
    uint32_t hours = (seconds % 86400) / 3600;
    uint32_t mins = (seconds % 3600) / 60;

    if (days > 0) {
        snprintf(buf, sizeof(buf), "%dj %dh %dm", days, hours, mins);
    } else if (hours > 0) {
        snprintf(buf, sizeof(buf), "%dh %dm", hours, mins);
    } else {
        snprintf(buf, sizeof(buf), "%dm", mins);
    }
    return String(buf);
}
