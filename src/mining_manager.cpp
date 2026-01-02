// mining_manager.cpp - Implémentation gestionnaire de mineurs
#include "mining_manager.h"

MiningManager miningManager;

MiningManager::MiningManager() {
    minerCount = 0;
    lastRefresh = 0;
    memset(miners, 0, sizeof(miners));
    memset(minerIPs, 0, sizeof(minerIPs));
}

void MiningManager::begin() {
    loadFromNVS();
    Serial.printf("Mining Manager: %d mineur(s) configuré(s)\n", minerCount);

    // Rafraîchir les données au démarrage
    if (minerCount > 0) {
        refreshAll();
    }
}

bool MiningManager::addMiner(const char* ip) {
    if (minerCount >= MAX_MINERS) {
        Serial.println("Erreur: nombre max de mineurs atteint");
        return false;
    }

    // Vérifier si l'IP existe déjà
    for (int i = 0; i < minerCount; i++) {
        if (strcmp(minerIPs[i], ip) == 0) {
            Serial.println("Erreur: mineur déjà ajouté");
            return false;
        }
    }

    // Tester la connexion au mineur
    BitaxeMinerInfo testInfo;
    if (!bitaxeAPI.fetchMinerInfo(ip, testInfo)) {
        Serial.printf("Erreur: impossible de contacter le mineur %s\n", ip);
        return false;
    }

    // Ajouter le mineur
    strncpy(minerIPs[minerCount], ip, sizeof(minerIPs[0]) - 1);
    miners[minerCount] = testInfo;
    minerCount++;

    // Sauvegarder
    saveToNVS();

    Serial.printf("Mineur ajouté: %s (%s) - %.2f GH/s\n",
                  testInfo.hostname, ip, testInfo.hashrate);

    return true;
}

bool MiningManager::removeMiner(int index) {
    if (index < 0 || index >= minerCount) {
        return false;
    }

    Serial.printf("Suppression mineur: %s\n", minerIPs[index]);

    // Décaler les mineurs suivants
    for (int i = index; i < minerCount - 1; i++) {
        strcpy(minerIPs[i], minerIPs[i + 1]);
        miners[i] = miners[i + 1];
    }

    minerCount--;
    memset(&miners[minerCount], 0, sizeof(BitaxeMinerInfo));
    memset(minerIPs[minerCount], 0, sizeof(minerIPs[0]));

    saveToNVS();
    return true;
}

void MiningManager::clearAllMiners() {
    minerCount = 0;
    memset(miners, 0, sizeof(miners));
    memset(minerIPs, 0, sizeof(minerIPs));
    saveToNVS();
    Serial.println("Tous les mineurs supprimés");
}

void MiningManager::refreshAll() {
    Serial.println("Rafraîchissement de tous les mineurs...");

    for (int i = 0; i < minerCount; i++) {
        refreshMiner(i);
        delay(100);  // Petit délai entre les requêtes
    }

    lastRefresh = millis();
}

bool MiningManager::refreshMiner(int index) {
    if (index < 0 || index >= minerCount) {
        return false;
    }

    return bitaxeAPI.fetchMinerInfo(minerIPs[index], miners[index]);
}

BitaxeMinerInfo* MiningManager::getMiner(int index) {
    if (index < 0 || index >= minerCount) {
        return nullptr;
    }
    return &miners[index];
}

MiningStats MiningManager::getAggregatedStats() {
    MiningStats stats;
    memset(&stats, 0, sizeof(stats));

    stats.minerCount = minerCount;

    if (minerCount == 0) {
        return stats;
    }

    float totalTemp = 0;
    int tempCount = 0;

    for (int i = 0; i < minerCount; i++) {
        if (miners[i].isOnline) {
            stats.onlineCount++;
            stats.totalHashrate += miners[i].hashrate;
            stats.totalPower += miners[i].power;
            stats.totalSharesAccepted += miners[i].sharesAccepted;
            stats.totalSharesRejected += miners[i].sharesRejected;

            if (miners[i].tempChip > 0) {
                totalTemp += miners[i].tempChip;
                tempCount++;
            }
        }
    }

    // Calculer les moyennes
    if (tempCount > 0) {
        stats.avgTemp = totalTemp / tempCount;
    }

    // Efficacité moyenne (J/TH)
    if (stats.totalHashrate > 0) {
        stats.avgEfficiency = (stats.totalPower * 1000.0f) / stats.totalHashrate;
    }

    stats.valid = true;
    return stats;
}

String MiningManager::formatMiningStatus() {
    if (minerCount == 0) {
        return "Aucun mineur configuré. Ajoutez des mineurs Bitaxe via le menu Mining.";
    }

    MiningStats stats = getAggregatedStats();

    String status = "";

    // Résumé global
    status += "Votre mine compte ";
    status += String(minerCount) + " mineur";
    if (minerCount > 1) status += "s";
    status += ", dont " + String(stats.onlineCount) + " en ligne. ";

    // Hashrate total
    if (stats.totalHashrate >= 1000) {
        status += "Hashrate total: " + String(stats.totalHashrate / 1000.0f, 2) + " térahash par seconde. ";
    } else {
        status += "Hashrate total: " + String(stats.totalHashrate, 1) + " gigahash par seconde. ";
    }

    // Consommation
    status += "Consommation: " + String(stats.totalPower, 1) + " watts. ";

    // Efficacité
    if (stats.avgEfficiency > 0) {
        status += "Efficacité moyenne: " + String(stats.avgEfficiency, 1) + " joules par térahash. ";
    }

    // Température moyenne
    if (stats.avgTemp > 0) {
        status += "Température moyenne: " + String(stats.avgTemp, 1) + " degrés. ";
    }

    // Shares
    float rejectRate = 0;
    if (stats.totalSharesAccepted > 0) {
        rejectRate = 100.0f * stats.totalSharesRejected / (stats.totalSharesAccepted + stats.totalSharesRejected);
    }
    status += "Shares acceptés: " + String(stats.totalSharesAccepted);
    if (rejectRate > 0.1) {
        status += ", rejetés: " + String(rejectRate, 1) + " pourcent";
    }
    status += ".";

    return status;
}

String MiningManager::formatTotalHashrate() {
    MiningStats stats = getAggregatedStats();

    if (stats.totalHashrate >= 1000) {
        return String(stats.totalHashrate / 1000.0f, 2) + " TH/s";
    } else {
        return String(stats.totalHashrate, 1) + " GH/s";
    }
}

String MiningManager::formatTotalPower() {
    MiningStats stats = getAggregatedStats();
    return String(stats.totalPower, 1) + " W";
}

String MiningManager::formatMinerDetails(int idx) {
    if (idx < 0 || idx >= minerCount) {
        return "Mineur non trouvé";
    }

    BitaxeMinerInfo& m = miners[idx];

    if (!m.isOnline) {
        return String(m.hostname) + " est hors ligne.";
    }

    String details = "";
    details += String(m.hostname) + ": ";
    details += bitaxeAPI.formatHashrate(m.hashrate) + ", ";
    details += String(m.power, 1) + "W, ";
    details += String(m.tempChip, 1) + "°C, ";
    details += "uptime " + bitaxeAPI.formatUptime(m.uptimeSeconds);

    return details;
}

bool MiningManager::needsRefresh() {
    return (millis() - lastRefresh) > MINER_REFRESH_INTERVAL;
}

void MiningManager::saveToNVS() {
    prefs.begin("mining", false);

    prefs.putInt("count", minerCount);

    for (int i = 0; i < minerCount; i++) {
        char key[16];
        snprintf(key, sizeof(key), "ip%d", i);
        prefs.putString(key, minerIPs[i]);
    }

    prefs.end();
    Serial.printf("Mining: %d mineur(s) sauvegardé(s)\n", minerCount);
}

void MiningManager::loadFromNVS() {
    prefs.begin("mining", true);  // Read-only

    minerCount = prefs.getInt("count", 0);
    if (minerCount > MAX_MINERS) minerCount = MAX_MINERS;

    for (int i = 0; i < minerCount; i++) {
        char key[16];
        snprintf(key, sizeof(key), "ip%d", i);
        String ip = prefs.getString(key, "");
        strncpy(minerIPs[i], ip.c_str(), sizeof(minerIPs[0]) - 1);
    }

    prefs.end();
    Serial.printf("Mining: %d mineur(s) chargé(s) depuis NVS\n", minerCount);
}
