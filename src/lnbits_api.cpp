// lnbits_api.cpp - Implementation API LNbits
#include "lnbits_api.h"
#include <ArduinoJson.h>

LNbitsAPI lnbitsAPI;

LNbitsAPI::LNbitsAPI() {
    client = nullptr;
    walletBalance.valid = false;
    walletBalance.balance = 0;
}

void LNbitsAPI::begin() {
    client = new WiFiClientSecure();
    client->setInsecure();
    Serial.println("LNbits API initialisee");
}

bool LNbitsAPI::fetchBalance() {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    // Verifier si LNbits est configure
    if (strlen(configManager.config.lnbits_host) == 0 ||
        strlen(configManager.config.lnbits_invoice_key) == 0) {
        lastError = "LNbits non configure";
        walletBalance.valid = false;
        return false;
    }

    HTTPClient https;
    https.setTimeout(10000);  // 10 secondes

    String url = String(configManager.config.lnbits_host) + "/api/v1/wallet";
    Serial.println("LNbits fetch: " + url);

    if (!https.begin(*client, url)) {
        lastError = "Connexion LNbits echouee";
        return false;
    }

    https.addHeader("X-Api-Key", configManager.config.lnbits_invoice_key);

    int httpCode = https.GET();
    Serial.printf("LNbits HTTP: %d\n", httpCode);

    if (httpCode != 200) {
        lastError = "HTTP " + String(httpCode);
        https.end();
        walletBalance.valid = false;
        return false;
    }

    String response = https.getString();
    https.end();

    Serial.println("LNbits response: " + response);

    // Parser JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "JSON error";
        walletBalance.valid = false;
        return false;
    }

    if (doc["balance"].is<int64_t>()) {
        walletBalance.balance = doc["balance"].as<int64_t>();
        walletBalance.valid = true;
    }

    if (doc["name"].is<String>()) {
        walletBalance.name = doc["name"].as<String>();
    }

    Serial.printf("LNbits balance: %lld msat (%s)\n", walletBalance.balance, walletBalance.name.c_str());

    return true;
}

String LNbitsAPI::formatBalance() {
    if (!walletBalance.valid) {
        return "N/A";
    }

    // Convertir millisats en sats
    int64_t sats = walletBalance.balance / 1000;

    if (sats >= 1000000) {
        // Format en millions
        return String((float)sats / 1000000.0, 2) + " M sats";
    } else if (sats >= 1000) {
        // Format en milliers
        return String((float)sats / 1000.0, 1) + " k sats";
    } else {
        return String(sats) + " sats";
    }
}

bool LNbitsAPI::createInvoice(int64_t amountSats, const char* memo, String& bolt11) {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    // Verifier si LNbits est configure
    if (strlen(configManager.config.lnbits_host) == 0 ||
        strlen(configManager.config.lnbits_invoice_key) == 0) {
        lastError = "LNbits non configure";
        return false;
    }

    HTTPClient https;
    https.setTimeout(15000);

    String url = String(configManager.config.lnbits_host) + "/api/v1/payments";
    Serial.println("LNbits createInvoice: " + url);

    if (!https.begin(*client, url)) {
        lastError = "Connexion LNbits echouee";
        return false;
    }

    https.addHeader("X-Api-Key", configManager.config.lnbits_invoice_key);
    https.addHeader("Content-Type", "application/json");

    // Creer le JSON de la requete
    JsonDocument reqDoc;
    reqDoc["out"] = false;  // false = invoice entrante (recevoir)
    reqDoc["amount"] = amountSats;
    reqDoc["memo"] = memo;

    String requestBody;
    serializeJson(reqDoc, requestBody);
    Serial.println("Request: " + requestBody);

    int httpCode = https.POST(requestBody);
    Serial.printf("LNbits HTTP: %d\n", httpCode);

    if (httpCode != 200 && httpCode != 201) {
        lastError = "HTTP " + String(httpCode);
        String resp = https.getString();
        Serial.println("Error response: " + resp);
        https.end();
        return false;
    }

    String response = https.getString();
    https.end();

    Serial.println("LNbits response: " + response);

    // Parser JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "JSON error";
        return false;
    }

    if (doc["payment_request"].is<String>()) {
        bolt11 = doc["payment_request"].as<String>();
        Serial.println("BOLT11: " + bolt11);
        return true;
    }

    lastError = "Pas de payment_request";
    return false;
}

bool LNbitsAPI::payInvoice(const char* bolt11) {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    // Verifier si LNbits est configure avec admin key
    if (strlen(configManager.config.lnbits_host) == 0 ||
        strlen(configManager.config.lnbits_admin_key) == 0) {
        lastError = "LNbits admin key requise";
        return false;
    }

    HTTPClient https;
    https.setTimeout(30000);  // 30 sec pour paiement

    String url = String(configManager.config.lnbits_host) + "/api/v1/payments";
    Serial.println("LNbits payInvoice: " + url);

    if (!https.begin(*client, url)) {
        lastError = "Connexion LNbits echouee";
        return false;
    }

    https.addHeader("X-Api-Key", configManager.config.lnbits_admin_key);
    https.addHeader("Content-Type", "application/json");

    // Creer le JSON de la requete
    JsonDocument reqDoc;
    reqDoc["out"] = true;  // true = paiement sortant
    reqDoc["bolt11"] = bolt11;

    String requestBody;
    serializeJson(reqDoc, requestBody);
    Serial.println("Pay request: " + requestBody);

    int httpCode = https.POST(requestBody);
    Serial.printf("LNbits HTTP: %d\n", httpCode);

    String response = https.getString();
    https.end();

    Serial.println("LNbits response: " + response);

    if (httpCode != 200 && httpCode != 201) {
        // Extraire message d'erreur
        JsonDocument doc;
        if (!deserializeJson(doc, response)) {
            if (doc["detail"].is<String>()) {
                lastError = doc["detail"].as<String>();
            } else {
                lastError = "HTTP " + String(httpCode);
            }
        } else {
            lastError = "HTTP " + String(httpCode);
        }
        return false;
    }

    return true;
}

bool LNbitsAPI::payLnAddress(const char* lnAddress, int64_t amountSats) {
    // Lightning Address format: user@domain.com
    // 1. Fetch LNURL from /.well-known/lnurlp/user
    // 2. Get invoice from callback URL
    // 3. Pay the invoice

    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    String addr = String(lnAddress);
    int atPos = addr.indexOf('@');
    if (atPos < 0) {
        lastError = "Format adresse invalide";
        return false;
    }

    String user = addr.substring(0, atPos);
    String domain = addr.substring(atPos + 1);

    HTTPClient https;
    https.setTimeout(10000);

    // Etape 1: Recuperer les infos LNURL
    String lnurlUrl = "https://" + domain + "/.well-known/lnurlp/" + user;
    Serial.println("LNURL fetch: " + lnurlUrl);

    if (!https.begin(*client, lnurlUrl)) {
        lastError = "Connexion LNURL echouee";
        return false;
    }

    int httpCode = https.GET();
    if (httpCode != 200) {
        lastError = "LNURL HTTP " + String(httpCode);
        https.end();
        return false;
    }

    String response = https.getString();
    https.end();

    JsonDocument lnurlDoc;
    if (deserializeJson(lnurlDoc, response)) {
        lastError = "LNURL JSON error";
        return false;
    }

    if (!lnurlDoc["callback"].is<String>()) {
        lastError = "Pas de callback LNURL";
        return false;
    }

    String callback = lnurlDoc["callback"].as<String>();
    int64_t minSendable = lnurlDoc["minSendable"].as<int64_t>() / 1000;  // millisats -> sats
    int64_t maxSendable = lnurlDoc["maxSendable"].as<int64_t>() / 1000;

    if (amountSats < minSendable || amountSats > maxSendable) {
        lastError = "Montant hors limites";
        return false;
    }

    // Etape 2: Recuperer l'invoice
    String invoiceUrl = callback + "?amount=" + String(amountSats * 1000);  // en millisats
    Serial.println("Invoice fetch: " + invoiceUrl);

    if (!https.begin(*client, invoiceUrl)) {
        lastError = "Connexion callback echouee";
        return false;
    }

    httpCode = https.GET();
    if (httpCode != 200) {
        lastError = "Callback HTTP " + String(httpCode);
        https.end();
        return false;
    }

    response = https.getString();
    https.end();

    JsonDocument invoiceDoc;
    if (deserializeJson(invoiceDoc, response)) {
        lastError = "Invoice JSON error";
        return false;
    }

    if (!invoiceDoc["pr"].is<String>()) {
        lastError = "Pas d'invoice PR";
        return false;
    }

    String bolt11 = invoiceDoc["pr"].as<String>();
    Serial.println("Got invoice: " + bolt11);

    // Etape 3: Payer l'invoice
    return payInvoice(bolt11.c_str());
}

bool LNbitsAPI::fetchStats() {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    // Verifier si LNbits est configure
    if (strlen(configManager.config.lnbits_host) == 0 ||
        strlen(configManager.config.lnbits_invoice_key) == 0) {
        lastError = "LNbits non configure";
        walletStats.valid = false;
        return false;
    }

    HTTPClient https;
    https.setTimeout(15000);

    // Recuperer l'historique des paiements
    String url = String(configManager.config.lnbits_host) + "/api/v1/payments?limit=100";
    Serial.println("LNbits fetchStats: " + url);

    if (!https.begin(*client, url)) {
        lastError = "Connexion LNbits echouee";
        return false;
    }

    https.addHeader("X-Api-Key", configManager.config.lnbits_invoice_key);

    int httpCode = https.GET();
    Serial.printf("LNbits stats HTTP: %d\n", httpCode);

    if (httpCode != 200) {
        lastError = "HTTP " + String(httpCode);
        https.end();
        walletStats.valid = false;
        return false;
    }

    String response = https.getString();
    https.end();

    // Parser JSON array
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "JSON error";
        walletStats.valid = false;
        return false;
    }

    // Calculer les stats
    walletStats.received7d = 0;
    walletStats.spent7d = 0;
    walletStats.received30d = 0;
    walletStats.spent30d = 0;
    walletStats.txCount7d = 0;
    walletStats.txCount30d = 0;

    // Timestamps pour 7j et 30j (en secondes)
    unsigned long now = millis() / 1000;  // Approximatif, pas de RTC
    // On va utiliser les timestamps des paiements relatifs au premier

    JsonArray payments = doc.as<JsonArray>();
    unsigned long latestTime = 0;

    // Premier pass: trouver le timestamp le plus recent
    for (JsonObject payment : payments) {
        if (payment["time"].is<unsigned long>()) {
            unsigned long t = payment["time"].as<unsigned long>();
            if (t > latestTime) latestTime = t;
        }
    }

    // Calculer les seuils
    unsigned long threshold7d = latestTime - (7 * 24 * 3600);
    unsigned long threshold30d = latestTime - (30 * 24 * 3600);

    for (JsonObject payment : payments) {
        unsigned long payTime = 0;
        if (payment["time"].is<unsigned long>()) {
            payTime = payment["time"].as<unsigned long>();
        }

        int64_t amount = 0;
        if (payment["amount"].is<int64_t>()) {
            amount = payment["amount"].as<int64_t>() / 1000;  // millisats -> sats
        }

        bool pending = false;
        if (payment["pending"].is<bool>()) {
            pending = payment["pending"].as<bool>();
        }

        // Ignorer les paiements en attente
        if (pending) continue;

        // 30 derniers jours
        if (payTime >= threshold30d) {
            walletStats.txCount30d++;
            if (amount > 0) {
                walletStats.received30d += amount;
            } else {
                walletStats.spent30d += -amount;
            }

            // 7 derniers jours
            if (payTime >= threshold7d) {
                walletStats.txCount7d++;
                if (amount > 0) {
                    walletStats.received7d += amount;
                } else {
                    walletStats.spent7d += -amount;
                }
            }
        }
    }

    walletStats.valid = true;
    Serial.printf("Stats 7j: +%lld/-%lld sats (%d tx)\n",
                  walletStats.received7d, walletStats.spent7d, walletStats.txCount7d);
    Serial.printf("Stats 30j: +%lld/-%lld sats (%d tx)\n",
                  walletStats.received30d, walletStats.spent30d, walletStats.txCount30d);

    return true;
}

String LNbitsAPI::formatStats() {
    if (!walletStats.valid) {
        return "Stats N/A";
    }

    char buf[200];
    snprintf(buf, sizeof(buf),
             "7j: +%lld/-%lld sats (%d tx)\n30j: +%lld/-%lld sats (%d tx)",
             walletStats.received7d, walletStats.spent7d, walletStats.txCount7d,
             walletStats.received30d, walletStats.spent30d, walletStats.txCount30d);
    return String(buf);
}

String LNbitsAPI::getLnurlPayUrl() {
    // LNbits expose un endpoint LNURL-pay pour chaque wallet
    // Format: https://HOST/lnurlp/api/v1/lnurl/WALLET_ID
    // Mais on n'a pas le wallet_id directement...
    // Alternative: utiliser l'extension lnurlp si activee

    if (strlen(configManager.config.lnbits_host) == 0) {
        return "";
    }

    // On retourne simplement l'URL du portail pour l'instant
    // L'utilisateur peut configurer une Lightning Address
    return String(configManager.config.lnbits_host);
}
