#pragma once

#include <vector>
#include "can_driver.h"

class MockDriver : public CanDriver {
public:
    std::vector<CanFrame> sent;

    bool init() override { return true; }

    bool read(CanFrame& /*frame*/) override {
        return false;
    }

    void send(const CanFrame& frame) override {
        sent.push_back(frame);
    }

    void reset() {
        sent.clear();
    }
};
