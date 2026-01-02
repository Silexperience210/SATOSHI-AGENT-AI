// claude_api.cpp - Implementation de l'API Claude avec contexte Bitcoin
#include "claude_api.h"
#include <ArduinoJson.h>

ClaudeAPI claudeAPI;

ClaudeAPI::ClaudeAPI() {
    client = nullptr;
    bitcoinContext = "";
    miningContext = "";
}

void ClaudeAPI::begin() {
    client = new WiFiClientSecure();
    client->setInsecure();
    Serial.println("Claude API initialisee");
}

void ClaudeAPI::updateBitcoinContext(const String& context) {
    bitcoinContext = context;
}

void ClaudeAPI::updateMiningContext(const String& context) {
    miningContext = context;
}

String ClaudeAPI::buildRequestBody(const char* message) {
    JsonDocument doc;

    doc["model"] = CLAUDE_MODEL;
    doc["max_tokens"] = 100;  // Limite basse pour reponses courtes

    // System prompt pour SATOSHI avec contexte Bitcoin temps reel
    String systemPrompt = "Tu es SATOSHI, assistant Bitcoin et mining. "
                         "REGLE ABSOLUE: Reponse en 1-2 phrases, MAX 260 caracteres. "
                         "Sois direct, pas de bavardage. Francais uniquement.\n\n"
                         "IMPORTANT - ACTIONS LIGHTNING:\n"
                         "1. PAIEMENT: Quand l'utilisateur demande d'envoyer des sats/bitcoin:\n"
                         "   ACTION:PAY:<adresse_lightning>:<montant_sats>\n"
                         "   Exemple: 'envoie 10 sats a bob@walletofsatoshi.com' ->\n"
                         "   ACTION:PAY:bob@walletofsatoshi.com:10\n"
                         "   J'envoie 10 sats a bob!\n\n"
                         "2. RECEPTION: Quand l'utilisateur demande de recevoir des sats:\n"
                         "   ACTION:RECEIVE:<montant_sats>\n"
                         "   Exemple: 'je veux recevoir 100 sats' ->\n"
                         "   ACTION:RECEIVE:100\n"
                         "   Voici ton QR code pour recevoir 100 sats!\n\n"
                         "3. STATS: Quand l'utilisateur demande ses stats wallet:\n"
                         "   ACTION:STATS\n"
                         "   Voici tes statistiques!\n\n"
                         "4. MINING: Pour questions sur la mine/mineurs Bitaxe:\n"
                         "   ACTION:MINING_STATUS\n"
                         "   Puis utilise les donnees mining ci-dessous pour repondre.\n\n"
                         "Mets TOUJOURS la ligne ACTION en PREMIERE ligne, puis ton message.\n"
                         "L'adresse peut etre Lightning Address (user@domain.com) ou BOLT11 (lnbc...).\n\n";

    // Ajouter le contexte Bitcoin si disponible
    if (bitcoinContext.length() > 0) {
        systemPrompt += "DONNEES BITCOIN EN TEMPS REEL (via mempool.space):\n";
        systemPrompt += bitcoinContext;
        systemPrompt += "\n";
    }

    // Ajouter le contexte Mining si disponible
    if (miningContext.length() > 0) {
        systemPrompt += "\nDONNEES MINING BITAXE EN TEMPS REEL:\n";
        systemPrompt += miningContext;
        systemPrompt += "\n";
    }

    systemPrompt += "\nUtilise ces donnees pour repondre aux questions.";

    doc["system"] = systemPrompt;

    JsonArray messages = doc["messages"].to<JsonArray>();
    JsonObject userMsg = messages.add<JsonObject>();
    userMsg["role"] = "user";
    userMsg["content"] = message;

    String output;
    serializeJson(doc, output);
    return output;
}

bool ClaudeAPI::parseResponse(const String& jsonResponse, String& content) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        lastError = "Erreur JSON: " + String(error.c_str());
        Serial.println(lastError);
        return false;
    }

    if (doc.containsKey("error")) {
        lastError = doc["error"]["message"].as<String>();
        Serial.println("Erreur API: " + lastError);
        return false;
    }

    if (doc.containsKey("content") && doc["content"].size() > 0) {
        content = doc["content"][0]["text"].as<String>();
        return true;
    }

    lastError = "Format de reponse invalide";
    return false;
}

bool ClaudeAPI::sendMessage(const char* userMessage, String& response) {
    if (!client) {
        lastError = "Client non initialise";
        return false;
    }

    if (strlen(configManager.config.anthropic_key) == 0) {
        lastError = "Cle API manquante";
        return false;
    }

    Serial.println("Envoi a Claude...");
    Serial.printf("Message: %s\n", userMessage);

    HTTPClient https;
    https.setTimeout(30000);  // 30 secondes timeout

    if (!https.begin(*client, CLAUDE_API_URL)) {
        lastError = "Connexion echouee";
        return false;
    }

    https.addHeader("Content-Type", "application/json");
    https.addHeader("x-api-key", configManager.config.anthropic_key);
    https.addHeader("anthropic-version", "2023-06-01");

    String requestBody = buildRequestBody(userMessage);
    Serial.println("Request body:");
    Serial.println(requestBody);

    int httpCode = https.POST(requestBody);

    Serial.printf("Code HTTP: %d\n", httpCode);

    if (httpCode != 200) {
        String errorBody = https.getString();
        Serial.println("Erreur response:");
        Serial.println(errorBody);
        lastError = "HTTP " + String(httpCode);
        https.end();
        return false;
    }

    String responseBody = https.getString();
    Serial.println("Response:");
    Serial.println(responseBody);

    https.end();

    return parseResponse(responseBody, response);
}
