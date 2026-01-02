// bitcoin_api.cpp - Implementation complete API Bitcoin via mempool.space
#include "bitcoin_api.h"
#include <ArduinoJson.h>

BitcoinAPI bitcoinAPI;

BitcoinAPI::BitcoinAPI() {
    client = nullptr;
    blockHeight = 0;
    currentPrice = {0, 0, 0, 0, 0, 0, 0, 0, false};
    currentFees = {0, 0, 0, 0, 0, false};
    currentBlock = {0, "", 0, 0, 0, 0, 0, false};
    mempoolInfo = {0, 0, 0, false};
    addressInfo = {"", 0, 0, 0, 0, false};
    txInfo = {"", false, 0, 0, 0, 0, false};
    hashRateInfo = {0, 0, 0, 0, 0, 0, false};
    lightningStats = {0, 0, 0, 0, 0, false};
}

void BitcoinAPI::begin() {
    client = new WiFiClientSecure();
    client->setInsecure();
    Serial.println("Bitcoin API (mempool.space) initialisee");
}

// ============================================================
// Methode HTTP generique
// ============================================================

String BitcoinAPI::httpGet(const String& endpoint) {
    if (!client) {
        lastError = "Client non initialise";
        return "";
    }

    HTTPClient https;
    String url = String(MEMPOOL_BASE_URL) + endpoint;

    if (!https.begin(*client, url)) {
        lastError = "Connexion echouee: " + endpoint;
        return "";
    }

    https.setTimeout(10000);
    int httpCode = https.GET();

    if (httpCode != 200) {
        lastError = "HTTP " + String(httpCode) + " sur " + endpoint;
        https.end();
        return "";
    }

    String response = https.getString();
    https.end();
    return response;
}

bool BitcoinAPI::isConnected() {
    return client != nullptr;
}

// ============================================================
// Prix
// ============================================================

bool BitcoinAPI::fetchPrice() {
    String response = httpGet("/v1/prices");
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON prix: " + String(error.c_str());
        return false;
    }

    currentPrice.usd = doc["USD"].as<float>();
    currentPrice.eur = doc["EUR"].as<float>();
    currentPrice.gbp = doc["GBP"].as<float>();
    currentPrice.cad = doc["CAD"].as<float>();
    currentPrice.chf = doc["CHF"].as<float>();
    currentPrice.aud = doc["AUD"].as<float>();
    currentPrice.jpy = doc["JPY"].as<float>();
    currentPrice.timestamp = millis();
    currentPrice.valid = true;

    Serial.printf("Prix BTC: $%.0f / %.0f EUR\n", currentPrice.usd, currentPrice.eur);
    return true;
}

String BitcoinAPI::formatPrice(const char* currency) {
    if (!currentPrice.valid) {
        return "Prix non disponible";
    }

    char buf[32];
    if (strcmp(currency, "EUR") == 0) {
        snprintf(buf, sizeof(buf), "%.0f EUR", currentPrice.eur);
    } else if (strcmp(currency, "GBP") == 0) {
        snprintf(buf, sizeof(buf), "%.0f GBP", currentPrice.gbp);
    } else if (strcmp(currency, "CHF") == 0) {
        snprintf(buf, sizeof(buf), "%.0f CHF", currentPrice.chf);
    } else if (strcmp(currency, "CAD") == 0) {
        snprintf(buf, sizeof(buf), "%.0f CAD", currentPrice.cad);
    } else if (strcmp(currency, "AUD") == 0) {
        snprintf(buf, sizeof(buf), "%.0f AUD", currentPrice.aud);
    } else if (strcmp(currency, "JPY") == 0) {
        snprintf(buf, sizeof(buf), "%.0f JPY", currentPrice.jpy);
    } else {
        snprintf(buf, sizeof(buf), "$%.0f", currentPrice.usd);
    }
    return String(buf);
}

// ============================================================
// Frais
// ============================================================

bool BitcoinAPI::fetchFees() {
    String response = httpGet("/v1/fees/recommended");
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON frais: " + String(error.c_str());
        return false;
    }

    currentFees.fastestFee = doc["fastestFee"].as<int>();
    currentFees.halfHourFee = doc["halfHourFee"].as<int>();
    currentFees.hourFee = doc["hourFee"].as<int>();
    currentFees.economyFee = doc["economyFee"].as<int>();
    currentFees.minimumFee = doc["minimumFee"].as<int>();
    currentFees.valid = true;

    Serial.printf("Frais: rapide=%d, 30min=%d, 1h=%d, eco=%d sat/vB\n",
                  currentFees.fastestFee, currentFees.halfHourFee,
                  currentFees.hourFee, currentFees.economyFee);
    return true;
}

String BitcoinAPI::formatFees() {
    if (!currentFees.valid) {
        return "Frais non disponibles";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "Rapide: %d sat/vB, Normal: %d, Eco: %d",
             currentFees.fastestFee, currentFees.hourFee, currentFees.economyFee);
    return String(buf);
}

// ============================================================
// Blocs
// ============================================================

bool BitcoinAPI::fetchBlockHeight() {
    String response = httpGet("/blocks/tip/height");
    if (response.isEmpty()) return false;

    blockHeight = response.toInt();
    Serial.printf("Hauteur bloc: %d\n", blockHeight);
    return true;
}

bool BitcoinAPI::fetchLatestBlock() {
    // D'abord recuperer le hash du dernier bloc
    String hashResponse = httpGet("/blocks/tip/hash");
    if (hashResponse.isEmpty()) return false;

    return fetchBlockByHash(hashResponse.c_str());
}

bool BitcoinAPI::fetchBlock(int height) {
    String endpoint = "/block-height/" + String(height);
    String hashResponse = httpGet(endpoint);
    if (hashResponse.isEmpty()) return false;

    return fetchBlockByHash(hashResponse.c_str());
}

bool BitcoinAPI::fetchBlockByHash(const char* hash) {
    String endpoint = "/block/" + String(hash);
    String response = httpGet(endpoint);
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON bloc: " + String(error.c_str());
        return false;
    }

    currentBlock.height = doc["height"].as<int>();
    currentBlock.hash = String(hash);
    currentBlock.timestamp = doc["timestamp"].as<unsigned long>();
    currentBlock.txCount = doc["tx_count"].as<int>();
    currentBlock.size = doc["size"].as<int>();
    currentBlock.weight = doc["weight"].as<int>();
    currentBlock.difficulty = doc["difficulty"].as<double>();
    currentBlock.valid = true;

    blockHeight = currentBlock.height;

    Serial.printf("Bloc #%d: %d tx, %d KB\n",
                  currentBlock.height, currentBlock.txCount, currentBlock.size / 1024);
    return true;
}

String BitcoinAPI::formatBlockInfo() {
    if (!currentBlock.valid) {
        return "Info bloc non disponible";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "Bloc #%d: %d transactions, %.1f MB",
             currentBlock.height, currentBlock.txCount, currentBlock.size / 1048576.0f);
    return String(buf);
}

// ============================================================
// Mempool
// ============================================================

bool BitcoinAPI::fetchMempoolInfo() {
    String response = httpGet("/mempool");
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON mempool: " + String(error.c_str());
        return false;
    }

    mempoolInfo.count = doc["count"].as<int>();
    mempoolInfo.vsize = doc["vsize"].as<long>();
    mempoolInfo.totalFee = doc["total_fee"].as<long long>();
    mempoolInfo.valid = true;

    Serial.printf("Mempool: %d tx, %.1f MB\n", mempoolInfo.count, mempoolInfo.vsize / 1000000.0f);
    return true;
}

String BitcoinAPI::formatMempoolInfo() {
    if (!mempoolInfo.valid) {
        return "Mempool non disponible";
    }
    char buf[128];
    float mbSize = mempoolInfo.vsize / 1000000.0f;
    snprintf(buf, sizeof(buf), "%d tx en attente (%.1f MB)", mempoolInfo.count, mbSize);
    return String(buf);
}

// ============================================================
// Adresses
// ============================================================

bool BitcoinAPI::fetchAddressInfo(const char* address) {
    String endpoint = "/address/" + String(address);
    String response = httpGet(endpoint);
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON adresse: " + String(error.c_str());
        return false;
    }

    addressInfo.address = String(address);

    // chain_stats pour les TX confirmees
    JsonObject chain = doc["chain_stats"];
    long long chainFunded = chain["funded_txo_sum"].as<long long>();
    long long chainSpent = chain["spent_txo_sum"].as<long long>();
    int chainTxCount = chain["tx_count"].as<int>();

    // mempool_stats pour les TX non confirmees
    JsonObject mempool = doc["mempool_stats"];
    long long mempoolFunded = mempool["funded_txo_sum"].as<long long>();
    long long mempoolSpent = mempool["spent_txo_sum"].as<long long>();
    int mempoolTxCount = mempool["tx_count"].as<int>();

    addressInfo.funded = chainFunded + mempoolFunded;
    addressInfo.spent = chainSpent + mempoolSpent;
    addressInfo.balance = addressInfo.funded - addressInfo.spent;
    addressInfo.txCount = chainTxCount + mempoolTxCount;
    addressInfo.valid = true;

    Serial.printf("Adresse: %.8f BTC (%d tx)\n", satsToBTC(addressInfo.balance), addressInfo.txCount);
    return true;
}

String BitcoinAPI::formatAddressInfo() {
    if (!addressInfo.valid) {
        return "Adresse non disponible";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "Solde: %.8f BTC (%d transactions)",
             satsToBTC(addressInfo.balance), addressInfo.txCount);
    return String(buf);
}

String BitcoinAPI::formatBalance(long long sats) {
    char buf[32];
    if (sats >= 100000000) {
        snprintf(buf, sizeof(buf), "%.4f BTC", satsToBTC(sats));
    } else if (sats >= 1000000) {
        snprintf(buf, sizeof(buf), "%.2f mBTC", sats / 100000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%lld sats", sats);
    }
    return String(buf);
}

// ============================================================
// Transactions
// ============================================================

bool BitcoinAPI::fetchTransaction(const char* txid) {
    String endpoint = "/tx/" + String(txid);
    String response = httpGet(endpoint);
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON tx: " + String(error.c_str());
        return false;
    }

    txInfo.txid = String(txid);
    txInfo.fee = doc["fee"].as<int>();
    txInfo.vsize = doc["vsize"].as<int>();
    txInfo.weight = doc["weight"].as<int>();

    if (doc.containsKey("status")) {
        txInfo.confirmed = doc["status"]["confirmed"].as<bool>();
        if (txInfo.confirmed) {
            txInfo.blockHeight = doc["status"]["block_height"].as<int>();
        } else {
            txInfo.blockHeight = 0;
        }
    }

    txInfo.valid = true;

    Serial.printf("TX: %s... fee=%d sats, vsize=%d\n",
                  String(txid).substring(0, 16).c_str(), txInfo.fee, txInfo.vsize);
    return true;
}

String BitcoinAPI::formatTransactionInfo() {
    if (!txInfo.valid) {
        return "Transaction non disponible";
    }
    char buf[128];
    float feeRate = (float)txInfo.fee / txInfo.vsize;
    if (txInfo.confirmed) {
        snprintf(buf, sizeof(buf), "Confirmee (bloc #%d), %d sats (%.1f sat/vB)",
                 txInfo.blockHeight, txInfo.fee, feeRate);
    } else {
        snprintf(buf, sizeof(buf), "Non confirmee, %d sats (%.1f sat/vB)",
                 txInfo.fee, feeRate);
    }
    return String(buf);
}

// ============================================================
// Mining / Hashrate
// ============================================================

bool BitcoinAPI::fetchHashRate() {
    String response = httpGet("/v1/mining/hashrate/3d");
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON hashrate: " + String(error.c_str());
        return false;
    }

    hashRateInfo.currentHashrate = doc["currentHashrate"].as<double>();
    hashRateInfo.currentDifficulty = doc["currentDifficulty"].as<double>();
    hashRateInfo.valid = true;

    Serial.printf("Hashrate: %.2f EH/s\n", hashRateInfo.currentHashrate / 1e18);

    // Recuperer aussi l'ajustement de difficulte
    String diffResponse = httpGet("/v1/mining/difficulty-adjustments");
    if (!diffResponse.isEmpty()) {
        JsonDocument diffDoc;
        if (!deserializeJson(diffDoc, diffResponse)) {
            if (diffDoc.is<JsonArray>() && diffDoc.size() > 0) {
                hashRateInfo.difficultyChange = diffDoc[0]["difficultyChange"].as<float>();
            }
        }
    }

    return true;
}

String BitcoinAPI::formatHashRate() {
    if (!hashRateInfo.valid) {
        return "Hashrate non disponible";
    }
    return formatHashrate(hashRateInfo.currentHashrate);
}

String BitcoinAPI::formatDifficulty() {
    if (!hashRateInfo.valid) {
        return "Difficulte non disponible";
    }
    char buf[64];
    double trillion = hashRateInfo.currentDifficulty / 1e12;
    snprintf(buf, sizeof(buf), "%.2f T", trillion);
    return String(buf);
}

// ============================================================
// Lightning Network
// ============================================================

bool BitcoinAPI::fetchLightningStats() {
    String response = httpGet("/v1/lightning/statistics/latest");
    if (response.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "Erreur JSON lightning: " + String(error.c_str());
        return false;
    }

    JsonObject latest = doc["latest"];
    lightningStats.channelCount = latest["channel_count"].as<int>();
    lightningStats.nodeCount = latest["node_count"].as<int>();
    lightningStats.totalCapacity = latest["total_capacity"].as<long long>();
    lightningStats.avgFeeRate = latest["avg_fee_rate"].as<int>();
    lightningStats.avgBaseFee = latest["avg_base_fee_mtokens"].as<int>();
    lightningStats.valid = true;

    Serial.printf("Lightning: %d noeuds, %d canaux, %.0f BTC capacite\n",
                  lightningStats.nodeCount, lightningStats.channelCount,
                  satsToBTC(lightningStats.totalCapacity));
    return true;
}

String BitcoinAPI::formatLightningStats() {
    if (!lightningStats.valid) {
        return "Stats Lightning non disponibles";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "%d noeuds, %d canaux, %.0f BTC",
             lightningStats.nodeCount, lightningStats.channelCount,
             satsToBTC(lightningStats.totalCapacity));
    return String(buf);
}

// ============================================================
// Utilitaires de formatage
// ============================================================

String BitcoinAPI::formatNumber(long long num) {
    char buf[32];
    if (num >= 1000000000000LL) {
        snprintf(buf, sizeof(buf), "%.2f T", num / 1e12);
    } else if (num >= 1000000000) {
        snprintf(buf, sizeof(buf), "%.2f G", num / 1e9);
    } else if (num >= 1000000) {
        snprintf(buf, sizeof(buf), "%.2f M", num / 1e6);
    } else if (num >= 1000) {
        snprintf(buf, sizeof(buf), "%.2f K", num / 1e3);
    } else {
        snprintf(buf, sizeof(buf), "%lld", num);
    }
    return String(buf);
}

String BitcoinAPI::formatHashrate(double hashrate) {
    char buf[32];
    if (hashrate >= 1e21) {
        snprintf(buf, sizeof(buf), "%.2f ZH/s", hashrate / 1e21);
    } else if (hashrate >= 1e18) {
        snprintf(buf, sizeof(buf), "%.2f EH/s", hashrate / 1e18);
    } else if (hashrate >= 1e15) {
        snprintf(buf, sizeof(buf), "%.2f PH/s", hashrate / 1e15);
    } else if (hashrate >= 1e12) {
        snprintf(buf, sizeof(buf), "%.2f TH/s", hashrate / 1e12);
    } else {
        snprintf(buf, sizeof(buf), "%.2f GH/s", hashrate / 1e9);
    }
    return String(buf);
}
