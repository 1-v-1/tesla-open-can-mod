#pragma once

#include "can_frame_types.h"

inline uint8_t readMuxID(const CanFrame& frame) {
    return frame.data[0] & 0x07;
}

inline bool isFSDSelectedInUI(const CanFrame& frame) {
    return (frame.data[4] >> 6) & 0x01;
}

inline void setSpeedProfileV12V13(CanFrame& frame, int profile) {
    frame.data[6] &= ~0x06;
    frame.data[6] |= (profile << 1);
}

inline void setBit(CanFrame& frame, int bit, bool value) {
    int byteIndex = bit / 8;
    int bitIndex = bit % 8;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if (value) {
        frame.data[byteIndex] |= mask;
    } else {
        frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}
