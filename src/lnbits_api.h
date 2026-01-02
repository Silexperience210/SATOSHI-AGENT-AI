// lnbits_api.h - API LNbits pour SATOSHI
#ifndef LNBITS_API_H
#define LNBITS_API_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

struct WalletBalance {
    bool valid;
    int64_t balance;  // en millisatoshis
    String name;
};

struct Invoice {
    bool valid;
    String payment_hash;
    String payment_request;  // BOLT11
    int64_t amount;          // en sats
};

// Statistiques de transactions
struct WalletStats {
    bool valid;
    int64_t received7d;   // sats recus 7 derniers jours
    int64_t spent7d;      // sats depenses 7 derniers jours
    int64_t received30d;  // sats recus 30 derniers jours
    int64_t spent30d;     // sats depenses 30 derniers jours
    int txCount7d;        // nombre de tx 7 jours
    int txCount30d;       // nombre de tx 30 jours
};

class LNbitsAPI {
public:
    LNbitsAPI();

    void begin();
    bool fetchBalance();
    WalletBalance getBalance() { return walletBalance; }
    String formatBalance();  // Format en sats

    // Fonctions Invoice
    bool createInvoice(int64_t amountSats, const char* memo, String& bolt11);
    bool payInvoice(const char* bolt11);
    bool payLnAddress(const char* lnAddress, int64_t amountSats);

    // Statistiques
    bool fetchStats();
    WalletStats getStats() { return walletStats; }
    String formatStats();  // Format lisible

    // LNURL-pay (recevoir sans montant fixe)
    String getLnurlPayUrl();  // Retourne l'URL LNURL-pay du wallet

    String getLastError() { return lastError; }

private:
    WiFiClientSecure* client;
    WalletBalance walletBalance;
    WalletStats walletStats;
    String lastError;
};

extern LNbitsAPI lnbitsAPI;

#endif
