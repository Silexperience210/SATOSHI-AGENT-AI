// rgb_led.cpp - Gestion LED RGB WS2812 pour Freenove ESP32-S3
#include "rgb_led.h"
#include "config.h"
#include "esp32-hal-rgb-led.h"  // Arduino ESP32 RGB LED driver (neopixelWrite)

RGBLed rgbLed;

RGBLed::RGBLed() {
    initialized = false;
}

void RGBLed::begin() {
    // neopixelWrite gere l'initialisation du pin automatiquement
    initialized = true;
    off();
    Serial.printf("RGB LED initialisee sur GPIO%d\n", PIN_RGB_LED);
}

// Envoyer un pixel WS2812 via rgbLedWrite (Arduino ESP32)
void RGBLed::sendPixel(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized) return;
    rgbLedWrite(PIN_RGB_LED, r, g, b);
}

void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    sendPixel(r, g, b);
}

void RGBLed::setOrange(uint8_t brightness) {
    // Orange = R:255, G:100, B:0 (ajuste par brightness)
    uint8_t r = (255 * brightness) / 255;
    uint8_t g = (100 * brightness) / 255;
    uint8_t b = 0;
    sendPixel(r, g, b);
}

void RGBLed::setGreen(uint8_t brightness) {
    sendPixel(0, brightness, 0);
}

void RGBLed::setRed(uint8_t brightness) {
    sendPixel(brightness, 0, 0);
}

void RGBLed::setBlue(uint8_t brightness) {
    sendPixel(0, 0, brightness);
}

void RGBLed::off() {
    sendPixel(0, 0, 0);
}

void RGBLed::pulseOrange(uint8_t amplitude) {
    // Amplitude de 0-255, on le mappe sur une luminosite
    // Avec un minimum pour qu'on voie toujours quelque chose
    uint8_t minBright = 10;
    uint8_t brightness = minBright + ((amplitude * (255 - minBright)) / 255);
    setOrange(brightness);
}
