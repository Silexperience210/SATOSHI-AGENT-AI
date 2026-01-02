// touch.cpp - Implementation FT6336G touch controller
#include "touch.h"

TouchController touch;

TouchController::TouchController() {
    wire = nullptr;
}

bool TouchController::begin() {
    // Reset touch controller
    pinMode(PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(300);

    // Initialize I2C for touch (partage avec ES8311 sur Wire/I2C_NUM_0)
    // Wire est deja initialise par audioManager.begin()
    wire = &Wire;
    // Ne pas appeler wire->begin() car deja fait par audio

    // Check if FT6336G responds
    wire->beginTransmission(FT6336G_ADDR);
    uint8_t error = wire->endTransmission();

    if (error == 0) {
        Serial.println("Touch FT6336G initialise");
        return true;
    } else {
        Serial.printf("Touch FT6336G non trouve (erreur %d)\n", error);
        return false;
    }
}

uint8_t TouchController::readRegister(uint8_t reg) {
    wire->beginTransmission(FT6336G_ADDR);
    wire->write(reg);
    wire->endTransmission(false);
    wire->requestFrom(FT6336G_ADDR, (uint8_t)1);
    if (wire->available()) {
        return wire->read();
    }
    return 0;
}

bool TouchController::touched() {
    uint8_t numTouches = readRegister(0x02);  // TD_STATUS register
    return (numTouches > 0 && numTouches < 5);
}

void TouchController::getPoint(int16_t &x, int16_t &y) {
    // Read touch point 1 (registers 0x03-0x06)
    wire->beginTransmission(FT6336G_ADDR);
    wire->write(0x03);
    wire->endTransmission(false);
    wire->requestFrom(FT6336G_ADDR, (uint8_t)4);

    if (wire->available() >= 4) {
        uint8_t xh = wire->read();
        uint8_t xl = wire->read();
        uint8_t yh = wire->read();
        uint8_t yl = wire->read();

        // Coordonnees brutes (mode portrait 240x320)
        int16_t raw_x = ((xh & 0x0F) << 8) | xl;
        int16_t raw_y = ((yh & 0x0F) << 8) | yl;

        // Transformation pour mode PAYSAGE (rotation=1, 90° horaire)
        // L'ecran affiche en 320x240, le touch est en 240x320
        x = raw_y;              // X paysage = Y portrait
        y = 240 - raw_x;        // Y paysage = 240 - X portrait
    }
}
