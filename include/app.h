#pragma once

#include <memory>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "handlers.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

#ifndef PIN_LED
#define PIN_LED 2
#endif

#if defined(HW4)
using SelectedHandler = HW4Handler;
#elif defined(HW3)
using SelectedHandler = HW3Handler;
#elif defined(LEGACY)
using SelectedHandler = LegacyHandler;
#else
#error "Define HW4, HW3 or LEGACY in build_flags"
#endif

static std::unique_ptr<CanDriver> appDriver;
static std::unique_ptr<CarManagerBase> appHandler;

static volatile bool frameReady = true;
static void canISR() { frameReady = true; }

template <typename Driver>
static void appSetup(std::unique_ptr<Driver> drv, const char *readyMsg)
{
    appHandler = std::make_unique<SelectedHandler>();
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000)
    {
    }

    appDriver = std::move(drv);
    if (!appDriver->init())
    {
        Serial.println("CAN init failed");
    }

    appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
    if constexpr (Driver::kSupportsISR)
    {
        appDriver->enableInterrupt(canISR);
    }

    Serial.println(readyMsg);
}

template <typename Driver>
static void appLoop()
{
    if constexpr (Driver::kSupportsISR)
    {
        if (!frameReady)
            return;
        frameReady = false;
    }

    CanFrame frame;
    while (appDriver->read(frame))
    {
        digitalWrite(PIN_LED, LOW);
        appHandler->handleMessage(frame, *appDriver);
    }
    digitalWrite(PIN_LED, HIGH);
}
