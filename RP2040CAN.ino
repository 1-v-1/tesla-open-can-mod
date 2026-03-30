/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    This project uses the MCP2515 library (https://github.com/autowp/arduino-mcp2515)
    Copyright (c) 2013 Seeed Technology Inc.
    Copyright (c) 2016 Dmitry
    Licensed under the MIT License. See README.md for the full license text.
*/

#include "memory"
#include <SPI.h>
#include <mcp2515.h>


#define LEGACY // for what car to compile: HW4, HW3, or LEGACY

#if defined(HW4)
#define HW HW4Handler
#elif defined(HW3)
#define HW HW3Handler
#elif defined(LEGACY)
#define HW LegacyHandler
#endif

bool enablePrint = true;


#define LED_PIN PIN_LED                // onboard red LED (GPIO13)
#define CAN_CS PIN_CAN_CS              // GPIO19 on this Feather
#define CAN_INT_PIN PIN_CAN_INTERRUPT  // GPIO22 (unused here; polling)
#define CAN_STBY PIN_CAN_STANDBY       // GPIO16
#define CAN_RESET PIN_CAN_RESET        // GPIO18

// HW4 FSD V14 options
#define ENABLE_APPROACHING_EMERGENCY_VEHICLE_DETECTION true
#define ENABLE_ISA_SPEED_CHIME_SUPPRESS false // suppresses ISA speed chime, but speed limit sign will be empty while driving

std::unique_ptr<MCP2515> mcp;

struct CarManagerBase {
  int speedProfile = 1;
  bool FSDEnabled = false;
  virtual void handelMessage(can_frame& frame);
};

inline uint8_t readMuxID(const can_frame& frame) {
  return frame.data[0] & 0x07;
}

inline bool isFSDSelectedInUI(const can_frame& frame) {
  return (frame.data[4] >> 6) & 0x01;
}

inline void setSpeedProfileV12V13(can_frame& frame, int profile) {
  frame.data[6] &= ~0x06;
  frame.data[6] |= (profile << 1);
}

inline void setBit(can_frame& frame, int bit, bool value) {
  // Determine which byte and which bit within that byte
  int byteIndex = bit / 8;
  int bitIndex = bit % 8;
  // Set the desired bit
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  if (value) {
    frame.data[byteIndex] |= mask;
  } else {
    frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
  }
}


struct LegacyHandler : public CarManagerBase {
  virtual void handelMessage(can_frame& frame) override {
    // STW_ACTN_RQ (0x045 = 69): Follow-Distance-Stalk as Source for Profile Mapping
    // byte[1]: 0x00=Pos1, 0x21=Pos2, 0x42=Pos3, 0x64=Pos4, 0x85=Pos5, 0xA6=Pos6, 0xC8=Pos7
    if (frame.can_id == 69) {
      uint8_t pos = frame.data[1] >> 5;
      if      (pos <= 1) speedProfile = 2; 
      else if (pos == 2) speedProfile = 1; 
      else               speedProfile = 0;  
      return;
    }
    if (frame.can_id == 1006) {
      auto index = readMuxID(frame);
      if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
      if (index == 0 && FSDEnabled) {
        setBit(frame, 46, true);
        setSpeedProfileV12V13(frame, speedProfile);
        mcp->sendMessage(&frame);
      }
      if (index == 1) {
        setBit(frame, 19, false);
        mcp->sendMessage(&frame);
      }
      if (index == 0 && enablePrint) {
        Serial.printf("LegacyHandler: FSD: %d, Profile: %d\n", FSDEnabled, speedProfile);
      }
    }
  }
};

struct HW3Handler : public CarManagerBase {
  int speedOffset = 0;
  virtual void handelMessage(can_frame& frame) override {
    if (frame.can_id == 1016) {
      uint8_t followDistance = (frame.data[5] & 0b11100000) >> 5;
      switch (followDistance) {
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
    if (frame.can_id == 1021) {
      auto index = readMuxID(frame);
      if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
      if (index == 0 && FSDEnabled) {
        speedOffset = std::max(std::min(((uint8_t)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
        auto off = (uint8_t)((frame.data[3] >> 1) & 0x3F) - 30;
        switch (off) {
          case 2: speedProfile = 2; break;
          case 1: speedProfile = 1; break;
          case 0: speedProfile = 0; break;
          default: break;
        }
        setBit(frame, 46, true);
        setSpeedProfileV12V13(frame, speedProfile);
        mcp->sendMessage(&frame);
      }
      if (index == 1) {
        setBit(frame, 19, false);
        mcp->sendMessage(&frame);
      }
      if (index == 2 && FSDEnabled) {
        frame.data[0] &= ~(0b11000000);
        frame.data[1] &= ~(0b00111111);
        frame.data[0] |= (speedOffset & 0x03) << 6;
        frame.data[1] |= (speedOffset >> 2);
        mcp->sendMessage(&frame);
      }
      if (index == 0 && enablePrint) {
        Serial.printf("HW3Handler: FSD: %d, Profile: %d, Offset: %d\n", FSDEnabled, speedProfile, speedOffset);
      }
    }
  }
};

struct HW4Handler : public CarManagerBase {
  virtual void handelMessage(can_frame& frame) override {
    if (ENABLE_ISA_SPEED_CHIME_SUPPRESS && frame.can_id == 921) {
      frame.data[1] |= 0x20;
      uint8_t sum = 0;
      for (int i = 0; i < 7; i++) sum += frame.data[i];
      sum += (921 & 0xFF) + (921 >> 8);
      frame.data[7] = sum & 0xFF;
      mcp->sendMessage(&frame);
      return;
    }
    if (frame.can_id == 1016) {
      auto fd = (frame.data[5] & 0b11100000) >> 5;
      switch(fd){
        case 1: speedProfile = 3; break;
        case 2: speedProfile = 2; break;
        case 3: speedProfile = 1; break;
        case 4: speedProfile = 0; break;
        case 5: speedProfile = 4; break;
      }
    }
    if (frame.can_id == 1021) {
      auto index = readMuxID(frame);
      if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
      if (index == 0 && FSDEnabled) {
        setBit(frame, 46, true);
        setBit(frame, 60, true);
        if (ENABLE_APPROACHING_EMERGENCY_VEHICLE_DETECTION) {
          setBit(frame, 59, true);
        }
        mcp->sendMessage(&frame);
      }
      if (index == 1) {
        setBit(frame, 19, false);
        setBit(frame, 47, true);
        mcp->sendMessage(&frame);
      }
      if(index == 2){
        frame.data[7] &= ~(0x07 << 4);
        frame.data[7] |= (speedProfile & 0x07) << 4;
        mcp->sendMessage(&frame);
      }
      if (index == 0 && enablePrint) {
        Serial.printf("HW4Handler: FSD: %d, profile: %d\n", FSDEnabled, speedProfile);
      }
    }
  }
};


std::unique_ptr<CarManagerBase> handler;


void setup() {
  handler = std::make_unique<HW>();
  delay(1500); 
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 1000) {}

  mcp = std::make_unique<MCP2515>(CAN_CS);

  mcp->reset();
  MCP2515::ERROR e = mcp->setBitrate(CAN_500KBPS, MCP_16MHZ);  
  if (e != MCP2515::ERROR_OK) Serial.println("setBitrate failed");
  mcp->setNormalMode();
  Serial.println("MCP25625 ready @ 500k 1");
}


__attribute__((optimize("O3"))) void loop() {
  can_frame frame;
  int r = mcp->readMessage(&frame);
  if (r != MCP2515::ERROR_OK) {
    digitalWrite(LED_PIN, HIGH);
    return;
  }
  digitalWrite(LED_PIN, LOW);
  handler->handelMessage(frame);
}
