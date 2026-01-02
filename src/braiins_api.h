// braiins_api.h - API Braiins Pool pour stats mining
#ifndef BRAIINS_API_H
#define BRAIINS_API_H

#include <Arduino.h>

// Structure pour les stats du pool
struct BraiinsPoolStats {
    bool valid;
    float luck;           // Luck du pool
    float hashrate5m;     // Hashrate 5 min (TH/s)
    float hashrate60m;    // Hashrate 60 min (TH/s)
    float hashrate24h;    // Hashrate 24h (TH/s)
    int activeWorkers;    // Nombre de workers actifs
    int blocksFound;      // Blocs trouves
};

// Structure pour le profil utilisateur
struct BraiinsProfile {
    bool valid;
    String username;
    float confirmedReward;  // Reward confirmee (BTC)
    float unconfirmedReward; // Reward non confirmee (BTC)
    float estimatedReward;   // Reward estimee (BTC)
    float hashrate5m;        // Hashrate utilisateur 5 min
    float hashrate60m;       // Hashrate utilisateur 60 min
    float hashrate24h;       // Hashrate utilisateur 24h
    int okWorkers;           // Workers OK
    int offWorkers;          // Workers offline
    int lowWorkers;          // Workers low hashrate
    int disWorkers;          // Workers disabled
};

// Structure pour un worker
struct BraiinsWorker {
    char name[64];
    bool isOnline;
    float hashrate5m;
    float hashrate60m;
    float hashrate24h;
    int sharesOk;
    int sharesStale;
    int sharesRejected;
    unsigned long lastShare;
};

#define MAX_BRAIINS_WORKERS 10

class BraiinsAPI {
public:
    BraiinsAPI();

    // Initialisation
    void begin();

    // Configuration du token API
    void setToken(const char* token);
    bool hasToken() { return strlen(apiToken) > 0; }

    // Recuperer les donnees
    bool fetchPoolStats();
    bool fetchProfile();
    bool fetchWorkers();
    bool fetchAll();

    // Accesseurs
    BraiinsPoolStats getPoolStats() { return poolStats; }
    BraiinsProfile getProfile() { return profile; }
    BraiinsWorker* getWorkers() { return workers; }
    int getWorkerCount() { return workerCount; }

    // Formatage pour affichage/TTS
    String formatPoolStatus();
    String formatProfileStatus();
    String formatWorkerStatus(int idx);
    String formatTotalStatus();  // Resume complet pour TTS

    // Erreur
    String getLastError() { return lastError; }

private:
    char apiToken[128];
    BraiinsPoolStats poolStats;
    BraiinsProfile profile;
    BraiinsWorker workers[MAX_BRAIINS_WORKERS];
    int workerCount;
    String lastError;

    // Helper pour requetes HTTP
    bool httpGet(const char* endpoint, String& response);

    // Formater hashrate
    String formatHashrate(float thps);
};

extern BraiinsAPI braiinsAPI;

#endif
