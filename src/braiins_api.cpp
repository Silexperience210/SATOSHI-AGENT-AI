// braiins_api.cpp - Implementation API Braiins Pool
#include "braiins_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

BraiinsAPI braiinsAPI;

// Base URL de l'API Braiins Pool
#define BRAIINS_API_BASE "https://pool.braiins.com"

BraiinsAPI::BraiinsAPI() {
    apiToken[0] = '\0';
    workerCount = 0;
    memset(&poolStats, 0, sizeof(poolStats));
    memset(&profile, 0, sizeof(profile));
}

void BraiinsAPI::begin() {
    // Charger le token depuis la config si disponible
    // Pour l'instant, le token doit etre configure via setToken()
    Serial.println("Braiins API initialisee");
}

void BraiinsAPI::setToken(const char* token) {
    strncpy(apiToken, token, sizeof(apiToken) - 1);
    apiToken[sizeof(apiToken) - 1] = '\0';
    Serial.printf("Braiins token configure: %s***\n",
                  strlen(token) > 4 ? String(token).substring(0, 4).c_str() : "");
}

bool BraiinsAPI::httpGet(const char* endpoint, String& response) {
    if (!hasToken()) {
        lastError = "Token non configure";
        return false;
    }

    HTTPClient http;
    String url = String(BRAIINS_API_BASE) + endpoint;

    Serial.printf("Braiins GET: %s\n", url.c_str());

    http.begin(url);
    http.addHeader("SlushPool-Auth-Token", apiToken);
    http.setTimeout(10000);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    } else {
        lastError = "HTTP " + String(httpCode);
        Serial.printf("Braiins erreur: %d\n", httpCode);
        http.end();
        return false;
    }
}

bool BraiinsAPI::fetchPoolStats() {
    String response;
    if (!httpGet("/stats/json/btc/", response)) {
        poolStats.valid = false;
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        lastError = "JSON parse error";
        poolStats.valid = false;
        return false;
    }

    // Parser les stats du pool
    poolStats.luck = doc["btc"]["luck_b10"] | doc["btc"]["luck_b50"] | 0.0f;
    poolStats.activeWorkers = doc["btc"]["active_workers"] | 0;

    // Hashrate du pool (en GH/s, convertir en TH/s)
    float poolHashGhs = doc["btc"]["pool_scoring_hash_rate"] | 0.0f;
    poolStats.hashrate5m = poolHashGhs / 1000.0f;  // TH/s

    poolStats.valid = true;
    Serial.println("Braiins pool stats OK");
    return true;
}

bool BraiinsAPI::fetchProfile() {
    String response;
    if (!httpGet("/accounts/profile/json/btc/", response)) {
        profile.valid = false;
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        lastError = "JSON parse error";
        profile.valid = false;
        return false;
    }

    // Parser le profil utilisateur
    profile.username = doc["username"] | "";

    // Rewards (en BTC)
    profile.confirmedReward = doc["btc"]["confirmed_reward"] | 0.0f;
    profile.unconfirmedReward = doc["btc"]["unconfirmed_reward"] | 0.0f;
    profile.estimatedReward = doc["btc"]["estimated_reward"] | 0.0f;

    // Hashrate (en GH/s dans l'API, convertir en TH/s)
    float hash5m = doc["btc"]["hash_rate_5m"] | 0.0f;
    float hash60m = doc["btc"]["hash_rate_60m"] | 0.0f;
    float hash24h = doc["btc"]["hash_rate_24h"] | 0.0f;

    // L'API retourne en H/s ou GH/s selon la version
    // On suppose GH/s et on convertit en TH/s
    if (hash5m > 1000000) {
        // Probablement en H/s
        profile.hashrate5m = hash5m / 1000000000000.0f;  // TH/s
        profile.hashrate60m = hash60m / 1000000000000.0f;
        profile.hashrate24h = hash24h / 1000000000000.0f;
    } else {
        // Probablement en GH/s
        profile.hashrate5m = hash5m / 1000.0f;  // TH/s
        profile.hashrate60m = hash60m / 1000.0f;
        profile.hashrate24h = hash24h / 1000.0f;
    }

    // Compteurs de workers
    profile.okWorkers = doc["btc"]["ok_workers"] | 0;
    profile.offWorkers = doc["btc"]["off_workers"] | 0;
    profile.lowWorkers = doc["btc"]["low_workers"] | 0;
    profile.disWorkers = doc["btc"]["dis_workers"] | 0;

    profile.valid = true;
    Serial.printf("Braiins profile OK: %s, %.8f BTC\n",
                  profile.username.c_str(), profile.confirmedReward);
    return true;
}

bool BraiinsAPI::fetchWorkers() {
    String response;
    if (!httpGet("/accounts/workers/json/btc/", response)) {
        workerCount = 0;
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        lastError = "JSON parse error";
        workerCount = 0;
        return false;
    }

    // Parser les workers
    JsonObject workersObj = doc["btc"]["workers"];
    workerCount = 0;

    for (JsonPair kv : workersObj) {
        if (workerCount >= MAX_BRAIINS_WORKERS) break;

        BraiinsWorker& w = workers[workerCount];
        strncpy(w.name, kv.key().c_str(), sizeof(w.name) - 1);
        w.name[sizeof(w.name) - 1] = '\0';

        JsonObject worker = kv.value();

        // Etat du worker
        String state = worker["state"] | "off";
        w.isOnline = (state == "ok" || state == "low");

        // Hashrate
        float h5m = worker["hash_rate_5m"] | 0.0f;
        float h60m = worker["hash_rate_60m"] | 0.0f;
        float h24h = worker["hash_rate_24h"] | 0.0f;

        // Convertir en TH/s si necessaire
        if (h5m > 1000000) {
            w.hashrate5m = h5m / 1000000000000.0f;
            w.hashrate60m = h60m / 1000000000000.0f;
            w.hashrate24h = h24h / 1000000000000.0f;
        } else {
            w.hashrate5m = h5m / 1000.0f;
            w.hashrate60m = h60m / 1000.0f;
            w.hashrate24h = h24h / 1000.0f;
        }

        // Shares
        w.sharesOk = worker["shares_ok"] | 0;
        w.sharesStale = worker["shares_stale"] | 0;
        w.sharesRejected = worker["shares_rejected"] | 0;

        // Last share (timestamp)
        w.lastShare = worker["last_share"] | 0;

        workerCount++;
    }

    Serial.printf("Braiins workers OK: %d workers\n", workerCount);
    return true;
}

bool BraiinsAPI::fetchAll() {
    bool ok = true;
    ok &= fetchProfile();  // Profile contient le plus d'infos utiles
    ok &= fetchWorkers();
    // fetchPoolStats() optionnel car moins utile pour l'utilisateur
    return ok;
}

String BraiinsAPI::formatHashrate(float thps) {
    if (thps >= 1000.0f) {
        return String(thps / 1000.0f, 2) + " PH/s";
    } else if (thps >= 1.0f) {
        return String(thps, 2) + " TH/s";
    } else if (thps >= 0.001f) {
        return String(thps * 1000.0f, 1) + " GH/s";
    } else {
        return String(thps * 1000000.0f, 0) + " MH/s";
    }
}

String BraiinsAPI::formatPoolStatus() {
    if (!poolStats.valid) return "Stats pool non disponibles";

    char buf[200];
    snprintf(buf, sizeof(buf),
             "Pool Braiins: %d workers actifs, Luck %.0f%%",
             poolStats.activeWorkers,
             poolStats.luck * 100.0f);
    return String(buf);
}

String BraiinsAPI::formatProfileStatus() {
    if (!profile.valid) return "Profil non disponible";

    char buf[300];
    float totalBtc = profile.confirmedReward + profile.unconfirmedReward;
    int totalSats = (int)(totalBtc * 100000000);

    snprintf(buf, sizeof(buf),
             "Braiins %s: %s (24h). Reward: %d sats (%d confirmes). Workers: %d OK, %d off",
             profile.username.c_str(),
             formatHashrate(profile.hashrate24h).c_str(),
             totalSats,
             (int)(profile.confirmedReward * 100000000),
             profile.okWorkers,
             profile.offWorkers);
    return String(buf);
}

String BraiinsAPI::formatWorkerStatus(int idx) {
    if (idx < 0 || idx >= workerCount) return "";

    BraiinsWorker& w = workers[idx];
    char buf[150];
    snprintf(buf, sizeof(buf),
             "%s: %s %s, %d shares",
             w.name,
             w.isOnline ? "EN LIGNE" : "HORS LIGNE",
             formatHashrate(w.hashrate5m).c_str(),
             w.sharesOk);
    return String(buf);
}

String BraiinsAPI::formatTotalStatus() {
    if (!profile.valid) return "Braiins non connecte";

    char buf[400];
    float totalBtc = profile.confirmedReward + profile.unconfirmedReward;
    int totalSats = (int)(totalBtc * 100000000);

    snprintf(buf, sizeof(buf),
             "Braiins Pool, compte %s. "
             "Hashrate %s. "
             "%d workers en ligne, %d hors ligne. "
             "Recompenses: %d satoshis dont %d confirmes.",
             profile.username.c_str(),
             formatHashrate(profile.hashrate24h).c_str(),
             profile.okWorkers,
             profile.offWorkers,
             totalSats,
             (int)(profile.confirmedReward * 100000000));
    return String(buf);
}
