// captive_portal.cpp - Implémentation du portail captif
#include "captive_portal.h"

CaptivePortal captivePortal;

CaptivePortal::CaptivePortal() {
    server = nullptr;
    dnsServer = nullptr;
    configComplete = false;
}

void CaptivePortal::begin() {
    Serial.println("Démarrage du portail captif...");

    // Démarrer le point d'accès
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("AP démarré - SSID: %s, IP: %s\n", AP_SSID, apIP.toString().c_str());

    // Démarrer le serveur DNS pour captive portal
    dnsServer = new DNSServer();
    dnsServer->start(53, "*", apIP);

    // Démarrer le serveur web
    server = new WebServer(80);
    setupRoutes();
    server->begin();

    Serial.println("Portail captif prêt");

    // Afficher les instructions sur l'écran
    display.showSetupMode(AP_SSID, AP_PASSWORD);
}

void CaptivePortal::stop() {
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }

    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }

    WiFi.softAPdisconnect(true);
    Serial.println("Portail captif arrêté");
}

void CaptivePortal::handleClient() {
    if (dnsServer) dnsServer->processNextRequest();
    if (server) server->handleClient();
}

bool CaptivePortal::isConfigurationComplete() {
    return configComplete;
}

void CaptivePortal::setupRoutes() {
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/save", HTTP_POST, [this]() { handleSave(); });
    server->on("/scan", HTTP_GET, [this]() { handleScan(); });
    server->onNotFound([this]() { handleNotFound(); });
}

void CaptivePortal::handleRoot() {
    server->send(200, "text/html", getConfigPage());
}

void CaptivePortal::handleSave() {
    // Récupérer les paramètres
    if (server->hasArg("ssid")) {
        strncpy(configManager.config.wifi_ssid, server->arg("ssid").c_str(), 63);
    }
    if (server->hasArg("password")) {
        strncpy(configManager.config.wifi_password, server->arg("password").c_str(), 63);
    }
    if (server->hasArg("anthropic")) {
        strncpy(configManager.config.anthropic_key, server->arg("anthropic").c_str(), 127);
    }
    if (server->hasArg("groq")) {
        strncpy(configManager.config.groq_key, server->arg("groq").c_str(), 127);
    }
    if (server->hasArg("google_tts")) {
        strncpy(configManager.config.google_tts_key, server->arg("google_tts").c_str(), 127);
    }
    if (server->hasArg("tts_provider")) {
        configManager.config.tts_provider = server->arg("tts_provider").toInt();
    }
    if (server->hasArg("lnbits_url")) {
        strncpy(configManager.config.lnbits_host, server->arg("lnbits_url").c_str(), 127);
    }
    if (server->hasArg("lnbits_invoice")) {
        strncpy(configManager.config.lnbits_invoice_key, server->arg("lnbits_invoice").c_str(), 63);
    }
    if (server->hasArg("lnbits_admin")) {
        strncpy(configManager.config.lnbits_admin_key, server->arg("lnbits_admin").c_str(), 63);
    }

    configManager.config.configured = true;
    configManager.save();

    server->send(200, "text/html", getSuccessPage());

    configComplete = true;
    Serial.println("Configuration sauvegardée via portail");
}

void CaptivePortal::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";

    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }

    json += "]";
    server->send(200, "application/json", json);
}

void CaptivePortal::handleNotFound() {
    // Rediriger vers la page principale (captive portal)
    server->sendHeader("Location", "http://192.168.4.1/", true);
    server->send(302, "text/plain", "");
}

String CaptivePortal::getConfigPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SATOSHI Setup</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            padding: 20px;
            color: #fff;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
        }
        .logo {
            text-align: center;
            margin-bottom: 30px;
        }
        .logo h1 {
            color: #f7931a;
            font-size: 2.5em;
            margin-bottom: 5px;
        }
        .logo p { color: #888; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
        }
        .card h2 {
            color: #f7931a;
            margin-bottom: 20px;
            font-size: 1.2em;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #ccc;
            font-size: 0.9em;
        }
        input, select {
            width: 100%;
            padding: 12px;
            border: 1px solid #444;
            border-radius: 8px;
            background: #2a2a4a;
            color: #fff;
            font-size: 16px;
        }
        input:focus {
            outline: none;
            border-color: #f7931a;
        }
        button {
            width: 100%;
            padding: 15px;
            background: #f7931a;
            color: #fff;
            border: none;
            border-radius: 8px;
            font-size: 1.1em;
            cursor: pointer;
            margin-top: 10px;
        }
        button:hover { background: #e8850f; }
        .scan-btn {
            background: #444;
            margin-bottom: 10px;
        }
        .optional { color: #666; font-size: 0.8em; }
        #networks { margin-bottom: 15px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">
            <h1>&#8383; SATOSHI</h1>
            <p>Assistant Bitcoin</p>
        </div>

        <form action="/save" method="POST">
            <div class="card">
                <h2>WiFi</h2>
                <button type="button" class="scan-btn" onclick="scanNetworks()">Scanner les reseaux</button>
                <div id="networks"></div>
                <div class="form-group">
                    <label>SSID</label>
                    <input type="text" name="ssid" id="ssid" required>
                </div>
                <div class="form-group">
                    <label>Mot de passe</label>
                    <input type="password" name="password">
                </div>
            </div>

            <div class="card">
                <h2>Claude AI (Anthropic)</h2>
                <div class="form-group">
                    <label>Cle API Anthropic</label>
                    <input type="password" name="anthropic" placeholder="sk-ant-...">
                </div>
            </div>

            <div class="card">
                <h2>Groq (Whisper STT + TTS)</h2>
                <div class="form-group">
                    <label>Cle API Groq</label>
                    <input type="password" name="groq" placeholder="gsk_...">
                </div>
            </div>

            <div class="card">
                <h2>Text-to-Speech (TTS)</h2>
                <div class="form-group">
                    <label>Provider TTS</label>
                    <select name="tts_provider">
                        <option value="0">Groq Orpheus (3600 tokens/jour)</option>
                        <option value="1">Google Cloud (4M chars/mois)</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Cle Google TTS <span class="optional">(optionnel si Groq)</span></label>
                    <input type="password" name="google_tts" placeholder="AIza...">
                </div>
            </div>

            <div class="card">
                <h2>LNbits <span class="optional">(optionnel)</span></h2>
                <div class="form-group">
                    <label>URL LNbits</label>
                    <input type="url" name="lnbits_url" placeholder="https://legend.lnbits.com">
                </div>
                <div class="form-group">
                    <label>Invoice Key</label>
                    <input type="text" name="lnbits_invoice">
                </div>
                <div class="form-group">
                    <label>Admin Key</label>
                    <input type="text" name="lnbits_admin">
                </div>
            </div>

            <button type="submit">Sauvegarder</button>
        </form>
    </div>

    <script>
        function scanNetworks() {
            document.getElementById('networks').innerHTML = 'Scan en cours...';
            fetch('/scan')
                .then(r => r.json())
                .then(data => {
                    let html = '<select onchange="document.getElementById(\'ssid\').value=this.value">';
                    html += '<option value="">-- Choisir un reseau --</option>';
                    data.forEach(n => {
                        html += '<option value="' + n.ssid + '">' + n.ssid + ' (' + n.rssi + ' dBm)</option>';
                    });
                    html += '</select>';
                    document.getElementById('networks').innerHTML = html;
                })
                .catch(() => {
                    document.getElementById('networks').innerHTML = 'Erreur de scan';
                });
        }
    </script>
</body>
</html>
)rawliteral";
}

String CaptivePortal::getSuccessPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SATOSHI - Configuration OK</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
            text-align: center;
        }
        .success {
            background: rgba(255,255,255,0.1);
            padding: 40px;
            border-radius: 20px;
        }
        .check {
            font-size: 4em;
            color: #4caf50;
            margin-bottom: 20px;
        }
        h1 { color: #f7931a; margin-bottom: 10px; }
        p { color: #888; }
    </style>
</head>
<body>
    <div class="success">
        <div class="check">&#10004;</div>
        <h1>Configuration sauvegardee!</h1>
        <p>SATOSHI va redemarrer...</p>
    </div>
</body>
</html>
)rawliteral";
}
