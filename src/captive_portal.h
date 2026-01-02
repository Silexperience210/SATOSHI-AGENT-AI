// captive_portal.h - Portail captif pour configuration initiale
#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "config.h"
#include "display.h"

class CaptivePortal {
public:
    CaptivePortal();

    void begin();
    void stop();
    void handleClient();
    bool isConfigurationComplete();

private:
    WebServer* server;
    DNSServer* dnsServer;
    bool configComplete;

    void setupRoutes();

    // Handlers HTTP
    void handleRoot();
    void handleSave();
    void handleScan();
    void handleNotFound();

    // Pages HTML
    String getConfigPage();
    String getSuccessPage();
};

extern CaptivePortal captivePortal;

#endif
