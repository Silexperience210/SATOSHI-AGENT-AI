// bitcoin_api.h - API Bitcoin complete via mempool.space
#ifndef BITCOIN_API_H
#define BITCOIN_API_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define MEMPOOL_BASE_URL "https://mempool.space/api"

// Structures de donnees

struct BitcoinPrice {
    float usd;
    float eur;
    float gbp;
    float cad;
    float chf;
    float aud;
    float jpy;
    unsigned long timestamp;
    bool valid;
};

struct BitcoinFees {
    int fastestFee;      // Prochain bloc
    int halfHourFee;     // ~30 minutes
    int hourFee;         // ~1 heure
    int economyFee;      // Economique
    int minimumFee;      // Minimum
    bool valid;
};

struct BlockInfo {
    int height;
    String hash;
    unsigned long timestamp;
    int txCount;
    int size;
    int weight;
    double difficulty;
    bool valid;
};

struct MempoolInfo {
    int count;           // Nombre de transactions
    long vsize;          // Taille totale en vbytes
    long long totalFee;  // Frais totaux en sats
    bool valid;
};

struct AddressInfo {
    String address;
    long long funded;    // Total recu en sats
    long long spent;     // Total depense en sats
    long long balance;   // Solde en sats
    int txCount;
    bool valid;
};

struct TransactionInfo {
    String txid;
    bool confirmed;
    int blockHeight;
    int fee;
    int vsize;
    int weight;
    bool valid;
};

struct HashRateInfo {
    double currentHashrate;     // H/s
    double currentDifficulty;
    int progressPercent;        // % vers prochain ajustement
    int remainingBlocks;
    int remainingTime;          // Secondes estimees
    float difficultyChange;     // Changement estime en %
    bool valid;
};

struct LightningStats {
    int channelCount;
    int nodeCount;
    long long totalCapacity;    // Capacite en sats
    int avgFeeRate;
    int avgBaseFee;
    bool valid;
};

struct UTXOInfo {
    String txid;
    int vout;
    long long value;    // sats
    bool confirmed;
};

class BitcoinAPI {
public:
    BitcoinAPI();

    void begin();

    // === Prix ===
    bool fetchPrice();
    BitcoinPrice getPrice() { return currentPrice; }
    String formatPrice(const char* currency = "USD");

    // === Frais ===
    bool fetchFees();
    BitcoinFees getFees() { return currentFees; }
    String formatFees();

    // === Blocs ===
    bool fetchBlockHeight();
    bool fetchLatestBlock();
    bool fetchBlock(int height);
    bool fetchBlockByHash(const char* hash);
    int getBlockHeight() { return blockHeight; }
    BlockInfo getBlockInfo() { return currentBlock; }
    String formatBlockInfo();

    // === Mempool ===
    bool fetchMempoolInfo();
    MempoolInfo getMempoolInfo() { return mempoolInfo; }
    String formatMempoolInfo();

    // === Adresses ===
    bool fetchAddressInfo(const char* address);
    AddressInfo getAddressInfo() { return addressInfo; }
    String formatAddressInfo();
    String formatBalance(long long sats);

    // === Transactions ===
    bool fetchTransaction(const char* txid);
    TransactionInfo getTransactionInfo() { return txInfo; }
    String formatTransactionInfo();

    // === Mining / Hashrate ===
    bool fetchHashRate();
    HashRateInfo getHashRateInfo() { return hashRateInfo; }
    String formatHashRate();
    String formatDifficulty();

    // === Lightning Network ===
    bool fetchLightningStats();
    LightningStats getLightningStats() { return lightningStats; }
    String formatLightningStats();

    // === Utilitaires ===
    String getLastError() { return lastError; }
    bool isConnected();

    // Conversion sats <-> BTC
    static float satsToBTC(long long sats) { return sats / 100000000.0f; }
    static long long btcToSats(float btc) { return (long long)(btc * 100000000); }

    // Formater les grands nombres
    static String formatNumber(long long num);
    static String formatHashrate(double hashrate);

private:
    WiFiClientSecure* client;
    String lastError;

    // Donnees en cache
    BitcoinPrice currentPrice;
    BitcoinFees currentFees;
    BlockInfo currentBlock;
    MempoolInfo mempoolInfo;
    AddressInfo addressInfo;
    TransactionInfo txInfo;
    HashRateInfo hashRateInfo;
    LightningStats lightningStats;
    int blockHeight;

    // Methode HTTP generique
    String httpGet(const String& endpoint);
};

extern BitcoinAPI bitcoinAPI;

#endif
