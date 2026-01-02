// bitaxe_api.h - API pour communication avec mineurs Bitaxe
// Endpoints: /api/system/info, /api/system/statistics
#ifndef BITAXE_API_H
#define BITAXE_API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Structure pour les infos d'un mineur Bitaxe
struct BitaxeMinerInfo {
    bool valid;
    char ip[32];
    char hostname[32];
    char version[16];
    char asicModel[32];

    // Performance
    float hashrate;        // GH/s
    float efficiency;      // J/TH
    float power;           // Watts
    float voltage;         // mV
    float frequency;       // MHz

    // Température
    float tempChip;        // °C
    float tempVrm;         // °C (si disponible)

    // Mining
    uint32_t sharesAccepted;
    uint32_t sharesRejected;
    uint32_t bestDiff;
    char bestDiffStr[32];

    // Pool
    char poolUrl[128];
    char poolUser[64];

    // Uptime
    uint32_t uptimeSeconds;

    // Statut
    bool isOnline;
    unsigned long lastUpdate;
};

// Stats agrégées de tous les mineurs
struct MiningStats {
    bool valid;
    int minerCount;
    int onlineCount;

    float totalHashrate;      // GH/s
    float totalPower;         // Watts
    float avgEfficiency;      // J/TH
    float avgTemp;            // °C

    uint32_t totalSharesAccepted;
    uint32_t totalSharesRejected;
};

#define MAX_MINERS 10

class BitaxeAPI {
public:
    BitaxeAPI();

    // Récupérer les infos d'un mineur
    bool fetchMinerInfo(const char* ip, BitaxeMinerInfo& info);

    // Récupérer les stats agrégées
    MiningStats getAggregatedStats();

    // Formater le hashrate pour affichage
    String formatHashrate(float ghps);

    // Formater l'uptime
    String formatUptime(uint32_t seconds);

    String getLastError() { return lastError; }

private:
    String lastError;

    // Parser la réponse JSON de /api/system/info
    bool parseSystemInfo(const String& json, BitaxeMinerInfo& info);
};

extern BitaxeAPI bitaxeAPI;

#endif
