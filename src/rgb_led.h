// rgb_led.h - Gestion LED RGB WS2812 pour Freenove ESP32-S3
#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>

class RGBLed {
public:
    RGBLed();

    void begin();

    // Couleurs predefinies
    void setOrange(uint8_t brightness = 255);
    void setGreen(uint8_t brightness = 255);
    void setRed(uint8_t brightness = 255);
    void setBlue(uint8_t brightness = 255);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void off();

    // Pulse avec amplitude audio (0-255)
    void pulseOrange(uint8_t amplitude);

private:
    bool initialized;
    void sendPixel(uint8_t r, uint8_t g, uint8_t b);
};

extern RGBLed rgbLed;

#endif
