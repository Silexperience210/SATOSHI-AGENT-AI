// touch.h - Support tactile FT6336G pour SATOSHI AGENT AI ES3C28P
#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

#define FT6336G_ADDR 0x38

class TouchController {
public:
    TouchController();

    bool begin();
    bool touched();
    void getPoint(int16_t &x, int16_t &y);

private:
    uint8_t readRegister(uint8_t reg);
    TwoWire* wire;
};

extern TouchController touch;

#endif
