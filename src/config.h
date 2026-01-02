// config.h - Configuration pour Freenove ESP32-S3 2.8" Touch Display (FNK0104)
// Source: https://docs.freenove.com/projects/fnk0104/en/latest/
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// ============================================================
// Configuration ecran - ILI9341V en mode PAYSAGE (320x240)
// ============================================================
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Pins ecran SPI (ILI9341V)
#define PIN_LCD_CS    10
#define PIN_LCD_DC    46
#define PIN_LCD_SCLK  12
#define PIN_LCD_MOSI  11
#define PIN_LCD_MISO  13
#define PIN_LCD_RST   -1    // Shared with ESP32-S3 reset
#define PIN_LCD_BL    45

// ============================================================
// Configuration Touch (I2C) - FT6336G
// ============================================================
#define PIN_TOUCH_SDA  16
#define PIN_TOUCH_SCL  15
#define PIN_TOUCH_RST  18
#define PIN_TOUCH_INT  17

// ============================================================
// Configuration Audio - I2S pour ES8311 (speaker + microphone)
// ES8311 = codec audio avec DAC (speaker) et ADC (microphone)
// Le micro MEMS analogique est connecte a l'ES8311 ADC
// Source: Freenove FNK0104 documentation
// ATTENTION: DIN et DOUT sont INVERSES par rapport a LCDWiki!
// ============================================================
#define PIN_I2S_MCLK   4    // MCLK - Audio I2S bus master clock
#define PIN_I2S_BCLK   5    // BCLK - Audio I2S bus bit clock
#define PIN_I2S_WS     7    // WS/LRCLK - Audio I2S bus left/right channel
#define PIN_I2S_DOUT   8    // Speaker output (vers ES8311 DAC) - GPIO8!
#define PIN_I2S_DIN    6    // Microphone input (depuis ES8311 ADC) - GPIO6!

// Audio amplifier enable (active LOW selon LCDWiki)
#define PIN_AUDIO_EN   1

// Configuration audio - 16000 Hz pour Whisper API
// Note: Freenove utilise 44100 Hz mais Whisper prefere 16kHz
#define AUDIO_SAMPLE_RATE  16000
#define AUDIO_BUFFER_SIZE  1024
#define MAX_RECORDING_TIME 15000

// ============================================================
// Configuration SD Card (SDMMC)
// ============================================================
#define PIN_SD_CLK    38
#define PIN_SD_CMD    40
#define PIN_SD_D0     39
#define PIN_SD_D1     41
#define PIN_SD_D2     48
#define PIN_SD_D3     47

// ============================================================
// Configuration RGB LED
// ============================================================
#define PIN_RGB_LED   42

// ============================================================
// Configuration batterie
// ============================================================
#define PIN_BAT_ADC   9

// ============================================================
// Configuration WiFi AP (Portail captif)
// ============================================================
#define AP_SSID     "SATOSHI-Setup"
#define AP_PASSWORD "bitcoin21"
#define AP_CHANNEL  1

// ============================================================
// TTS Providers
// ============================================================
#define TTS_PROVIDER_GROQ   0
#define TTS_PROVIDER_GOOGLE 1

// ============================================================
// Structure de configuration
// ============================================================
struct SatoshiConfig {
    char wifi_ssid[64];
    char wifi_password[64];
    char anthropic_key[128];
    char groq_key[128];        // Pour Groq Whisper (STT) et Groq TTS
    char google_tts_key[128];  // Pour Google Cloud TTS (optionnel)
    char lnbits_host[128];
    char lnbits_invoice_key[128];
    char lnbits_admin_key[128];
    char braiins_token[128];   // Token API Braiins Pool
    uint8_t tts_provider;      // 0=Groq, 1=Google
    bool configured;
};

// ============================================================
// Gestionnaire de configuration
// ============================================================
class ConfigManager {
public:
    SatoshiConfig config;

    void begin();
    void save();
    void reset();
    bool isConfigured();

private:
    Preferences prefs;
};

extern ConfigManager configManager;

#endif
