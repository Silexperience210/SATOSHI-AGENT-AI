// mining_manager.h - Gestionnaire de mineurs Bitaxe
// Stockage des IPs en NVS, agrégation des stats
#ifndef MINING_MANAGER_H
#define MINING_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "bitaxe_api.h"

#define MAX_MINERS 10
#define MINER_REFRESH_INTERVAL 30000  // Rafraîchir toutes les 30 secondes

class MiningManager {
public:
    MiningManager();

    // Initialisation (charge les IPs depuis NVS)
    void begin();

    // Gestion des mineurs
    bool addMiner(const char* ip);
    bool removeMiner(int index);
    void clearAllMiners();

    // Rafraîchir les données de tous les mineurs
    void refreshAll();

    // Rafraîchir un mineur spécifique
    bool refreshMiner(int index);

    // Obtenir les données
    int getMinerCount() { return minerCount; }
    BitaxeMinerInfo* getMiner(int index);
    BitaxeMinerInfo* getMiners() { return miners; }

    // Stats agrégées
    MiningStats getAggregatedStats();

    // Formater pour affichage/TTS
    String formatMiningStatus();        // Résumé complet pour TTS
    String formatTotalHashrate();       // Hashrate total
    String formatTotalPower();          // Consommation totale
    String formatMinerDetails(int idx); // Détails d'un mineur

    // Vérifier si besoin de rafraîchir
    bool needsRefresh();

    // Sauvegarde/Chargement NVS
    void saveToNVS();
    void loadFromNVS();

private:
    Preferences prefs;
    BitaxeMinerInfo miners[MAX_MINERS];
    char minerIPs[MAX_MINERS][32];
    int minerCount;
    unsigned long lastRefresh;
};

extern MiningManager miningManager;

#endif
