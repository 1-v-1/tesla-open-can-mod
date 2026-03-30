#pragma once

#include "can_frame_types.h"

struct CanDriver {
    virtual bool init() = 0;
    virtual bool read(CanFrame& frame) = 0;
    virtual void send(const CanFrame& frame) = 0;
    virtual ~CanDriver() = default;
};
