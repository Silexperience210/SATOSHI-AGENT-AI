// claude_api.h - Interface avec l'API Claude d'Anthropic
#ifndef CLAUDE_API_H
#define CLAUDE_API_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

#define CLAUDE_API_URL "https://api.anthropic.com/v1/messages"
#define CLAUDE_MODEL "claude-sonnet-4-20250514"
#define MAX_RESPONSE_LENGTH 2048

class ClaudeAPI {
public:
    ClaudeAPI();

    void begin();

    // Envoyer un message avec contexte Bitcoin
    bool sendMessage(const char* userMessage, String& response);

    // Mettre a jour le contexte Bitcoin (prix, fees, etc.)
    void updateBitcoinContext(const String& context);

    // Mettre a jour le contexte Mining (Bitaxe)
    void updateMiningContext(const String& context);

    String getLastError() { return lastError; }

private:
    WiFiClientSecure* client;
    String lastError;
    String bitcoinContext;  // Contexte Bitcoin actuel
    String miningContext;   // Contexte Mining (Bitaxe)

    String buildRequestBody(const char* message);
    bool parseResponse(const String& jsonResponse, String& content);
};

extern ClaudeAPI claudeAPI;

#endif
