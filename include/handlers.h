#pragma once

#include <memory>
#include <algorithm>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "shared_types.h"
#include "log_buffer.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

inline LogRingBuffer logRing;

struct CarManagerBase
{
    Shared<int> speedProfile{1};
    Shared<bool> FSDEnabled{false};
    Shared<bool> enablePrint{true};
    Shared<uint32_t> frameCount{0};
    Shared<uint32_t> framesSent{0};
    Shared<int> speedOffset{0};
    virtual void handleMessage(CanFrame &frame, CanDriver &driver) = 0;
    virtual const uint32_t *filterIds() const = 0;
    virtual uint8_t filterIdCount() const = 0;
    virtual ~CarManagerBase() = default;
};

struct LegacyHandler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {69, 1006};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        // STW_ACTN_RQ (0x045 = 69): Follow-Distance-Stalk as Source for Profile Mapping
        // byte[1]: 0x00=Pos1, 0x21=Pos2, 0x42=Pos3, 0x64=Pos4, 0x85=Pos5, 0xA6=Pos6, 0xC8=Pos7
        if (frame.id == 69)
        {
            if (frame.dlc < 2)
                return;
            uint8_t pos = frame.data[1] >> 5;
            if (pos <= 1)
                speedProfile = 2;
            else if (pos == 2)
                speedProfile = 1;
            else
                speedProfile = 0;
            return;
        }
        if (frame.id == 1006)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
                FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled)
            {
                setBit(frame, 46, true);
                setSpeedProfileV12V13(frame, speedProfile);
                framesSent++;
                driver.send(frame);
            }
            if (index == 1)
            {
                setBit(frame, 19, false);
                framesSent++;
                driver.send(frame);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "LegacyHandler: FSD: %d, Profile: %d",
                         (bool)FSDEnabled, (int)speedProfile);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};

struct HW3Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {1016, 1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            uint8_t followDistance = (frame.data[5] & 0b11100000) >> 5;
            switch (followDistance)
            {
            case 1:
                speedProfile = 2;
                break;
            case 2:
                speedProfile = 1;
                break;
            case 3:
                speedProfile = 0;
                break;
            default:
                break;
            }
            return;
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
                FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled)
            {
                speedOffset = std::max(std::min(((uint8_t)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
                auto off = (uint8_t)((frame.data[3] >> 1) & 0x3F) - 30;
                switch (off)
                {
                case 2:
                    speedProfile = 2;
                    break;
                case 1:
                    speedProfile = 1;
                    break;
                case 0:
                    speedProfile = 0;
                    break;
                default:
                    break;
                }
                setBit(frame, 46, true);
                setSpeedProfileV12V13(frame, speedProfile);
                framesSent++;
                driver.send(frame);
            }
            if (index == 1)
            {
                setBit(frame, 19, false);
                framesSent++;
                driver.send(frame);
            }
            if (index == 2 && FSDEnabled)
            {
                frame.data[0] &= ~(0b11000000);
                frame.data[1] &= ~(0b00111111);
                frame.data[0] |= (speedOffset & 0x03) << 6;
                frame.data[1] |= (speedOffset >> 2);
                framesSent++;
                driver.send(frame);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW3Handler: FSD: %d, Profile: %d, Offset: %d",
                         (bool)FSDEnabled, (int)speedProfile, (int)speedOffset);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};

struct HW4Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
#if defined(ISA_SPEED_CHIME_SUPPRESS)
        static constexpr uint32_t ids[] = {921, 1016, 1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 3; }
#else
        static constexpr uint32_t ids[] = {1016, 1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }
#endif

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
#if defined(ISA_SPEED_CHIME_SUPPRESS)
        if (frame.id == 921)
        {
            if (frame.dlc < 8)
                return;
            frame.data[1] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++)
                sum += frame.data[i];
            sum += (921 & 0xFF) + (921 >> 8);
            frame.data[7] = sum & 0xFF;
            framesSent++;
            driver.send(frame);
            return;
        }
#endif
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            auto fd = (frame.data[5] & 0b11100000) >> 5;
            switch (fd)
            {
            case 1:
                speedProfile = 3;
                break;
            case 2:
                speedProfile = 2;
                break;
            case 3:
                speedProfile = 1;
                break;
            case 4:
                speedProfile = 0;
                break;
            case 5:
                speedProfile = 4;
                break;
            }
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
                FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled)
            {
                setBit(frame, 46, true);
                setBit(frame, 60, true);
#if defined(EMERGENCY_VEHICLE_DETECTION)
                setBit(frame, 59, true);
#endif
                framesSent++;
                driver.send(frame);
            }
            if (index == 1)
            {
                setBit(frame, 19, false);
                setBit(frame, 47, true);
                framesSent++;
                driver.send(frame);
            }
            if (index == 2)
            {
                frame.data[7] &= ~(0x07 << 4);
                frame.data[7] |= (speedProfile & 0x07) << 4;
                framesSent++;
                driver.send(frame);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW4Handler: FSD: %d, Profile: %d",
                         (bool)FSDEnabled, (int)speedProfile);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};
