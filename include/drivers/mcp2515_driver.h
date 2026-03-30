#pragma once

#include <SPI.h>
#include <mcp2515.h>
#include <memory>
#include "can_driver.h"

class MCP2515Driver : public CanDriver {
public:
    explicit MCP2515Driver(uint8_t csPin) : mcp_(csPin) {}

    bool init() override {
        mcp_.reset();
        MCP2515::ERROR e = mcp_.setBitrate(CAN_500KBPS, MCP_16MHZ);
        if (e != MCP2515::ERROR_OK) return false;
        mcp_.setNormalMode();
        return true;
    }

    bool read(CanFrame& frame) override {
        can_frame raw;
        if (mcp_.readMessage(&raw) != MCP2515::ERROR_OK) return false;
        frame.id  = raw.can_id;
        frame.dlc = raw.can_dlc;
        memcpy(frame.data, raw.data, 8);
        return true;
    }

    void send(const CanFrame& frame) override {
        can_frame raw;
        raw.can_id  = frame.id;
        raw.can_dlc = frame.dlc;
        memcpy(raw.data, frame.data, 8);
        mcp_.sendMessage(&raw);
    }

private:
    MCP2515 mcp_;
};
