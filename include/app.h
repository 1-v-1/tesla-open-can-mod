#pragma once

#include <memory>
#include "can_frame_types.h"
#include "can_driver.h"
#include "can_helpers.h"
#include "handlers.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

#if defined(HW4)
using SelectedHandler = HW4Handler;
#elif defined(HW3)
using SelectedHandler = HW3Handler;
#else
using SelectedHandler = LegacyHandler;
#endif

static std::unique_ptr<CanDriver> appDriver;
static std::unique_ptr<CarManagerBase> appHandler;

static void appSetup(std::unique_ptr<CanDriver> drv, const char* readyMsg) {
    appHandler = std::make_unique<SelectedHandler>();
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000) {}

    appDriver = std::move(drv);
    if (!appDriver->init()) {
        Serial.println("CAN init failed");
    }
    Serial.println(readyMsg);
}

static void appLoop() {
    CanFrame frame;
    if (!appDriver->read(frame)) {
        digitalWrite(PIN_LED, HIGH);
        return;
    }
    digitalWrite(PIN_LED, LOW);
    appHandler->handleMessage(frame, *appDriver);
}
