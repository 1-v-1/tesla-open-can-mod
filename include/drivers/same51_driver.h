#pragma once

#include <CANSAME5x.h>
#include "can_driver.h"

class SAME51Driver : public CanDriver {
public:
    bool init() override {
        pinMode(PIN_CAN_STANDBY, OUTPUT);
        digitalWrite(PIN_CAN_STANDBY, false);
        pinMode(PIN_CAN_BOOSTEN, OUTPUT);
        digitalWrite(PIN_CAN_BOOSTEN, true);

        if (!can_.begin(500000)) return false;
        return true;
    }

    bool read(CanFrame& frame) override {
        int packetSize = can_.parsePacket();
        if (packetSize <= 0) return false;

        frame.id  = can_.packetId();
        frame.dlc = static_cast<uint8_t>(packetSize);
        for (int i = 0; i < packetSize && i < 8; i++) {
            frame.data[i] = static_cast<uint8_t>(can_.read());
        }
        return true;
    }

    void send(const CanFrame& frame) override {
        can_.beginPacket(frame.id);
        can_.write(frame.data, frame.dlc);
        can_.endPacket();
    }

private:
    CANSAME5x can_;
};
