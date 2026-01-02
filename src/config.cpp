// config.cpp - Implementation configuration SATOSHI AGENT AI
#include "config.h"

ConfigManager configManager;

// Cle Google TTS par defaut (Vertex AI) - SUPPRIMEE pour securite
#define DEFAULT_GOOGLE_TTS_KEY ""

void ConfigManager::begin() {
    prefs.begin("satoshi", false);

    // Charger depuis la memoire flash (NVS)
    String ssid = prefs.getString("wifi_ssid", "");
    String pass = prefs.getString("wifi_pass", "");
    String anthropic = prefs.getString("anthropic", "");
    String groq = prefs.getString("groq", "");
    String googleTts = prefs.getString("google_tts", DEFAULT_GOOGLE_TTS_KEY);
    String lnHost = prefs.getString("lnbits_host", "");
    String lnInv = prefs.getString("lnbits_inv", "");
    String lnAdm = prefs.getString("lnbits_adm", "");
    String braiins = prefs.getString("braiins", "");
    uint8_t ttsProvider = prefs.getUChar("tts_provider", TTS_PROVIDER_GROQ);

    strncpy(config.wifi_ssid, ssid.c_str(), 63);
    strncpy(config.wifi_password, pass.c_str(), 63);
    strncpy(config.anthropic_key, anthropic.c_str(), 127);
    strncpy(config.groq_key, groq.c_str(), 127);
    strncpy(config.google_tts_key, googleTts.c_str(), 127);
    strncpy(config.lnbits_host, lnHost.c_str(), 127);
    strncpy(config.lnbits_invoice_key, lnInv.c_str(), 127);
    strncpy(config.lnbits_admin_key, lnAdm.c_str(), 127);
    strncpy(config.braiins_token, braiins.c_str(), 127);
    config.tts_provider = ttsProvider;
    config.configured = prefs.getBool("configured", false);

    Serial.println("Configuration chargee depuis NVS");
    Serial.printf("WiFi: %s\n", config.wifi_ssid);
    Serial.printf("TTS Provider: %s\n", ttsProvider == TTS_PROVIDER_GOOGLE ? "Google" : "Groq");
    Serial.printf("Configured: %s\n", config.configured ? "oui" : "non");
}

void ConfigManager::save() {
    prefs.putString("wifi_ssid", config.wifi_ssid);
    prefs.putString("wifi_pass", config.wifi_password);
    prefs.putString("anthropic", config.anthropic_key);
    prefs.putString("groq", config.groq_key);
    prefs.putString("google_tts", config.google_tts_key);
    prefs.putString("lnbits_host", config.lnbits_host);
    prefs.putString("lnbits_inv", config.lnbits_invoice_key);
    prefs.putString("lnbits_adm", config.lnbits_admin_key);
    prefs.putString("braiins", config.braiins_token);
    prefs.putUChar("tts_provider", config.tts_provider);
    prefs.putBool("configured", true);
    config.configured = true;

    Serial.println("Configuration sauvegardee");
}

void ConfigManager::reset() {
    prefs.clear();
    memset(&config, 0, sizeof(config));
    config.configured = false;
    Serial.println("Configuration reinitalisee");
}

bool ConfigManager::isConfigured() {
    return config.configured &&
           strlen(config.wifi_ssid) > 0 &&
           strlen(config.anthropic_key) > 0;
}
