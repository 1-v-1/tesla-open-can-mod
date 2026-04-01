#pragma once

#include "../can_frame_types.h"
#include "can_driver.h"
#include <driver/twai.h>

class TWAIDriver : public CanDriver
{
public:
    static constexpr bool kSupportsISR = false;

    TWAIDriver(gpio_num_t txPin, gpio_num_t rxPin)
        : txPin_(txPin), rxPin_(rxPin) {}

    bool init() override
    {
        g_config_ = TWAI_GENERAL_CONFIG_DEFAULT(txPin_, rxPin_, TWAI_MODE_NORMAL);
        g_config_.rx_queue_len = 32;
        g_config_.tx_queue_len = 16;

        t_config_ = TWAI_TIMING_CONFIG_500KBITS();
        f_config_ = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config_, &t_config_, &f_config_) != ESP_OK)
            return false;
        if (twai_start() != ESP_OK)
            return false;
        return true;
    }

    void setFilters(const uint32_t *ids, uint8_t count) override
    {
        if (count == 0)
            return;

        // Compute combined mask: bits that differ between any pair of IDs
        // are "don't care" in the acceptance mask.
        uint32_t differ = 0;
        for (uint8_t i = 1; i < count; i++)
        {
            differ |= ids[0] ^ ids[i];
        }

        // TWAI standard-frame single-filter layout:
        //   acceptance_code bits [31:21] = standard ID [10:0]
        //   acceptance_mask: 1 = don't care, 0 = must match
        //   Lower 21 bits set to 1 (don't care about RTR, data bytes)
        uint32_t base = ids[0] & ~differ;
        f_config_.acceptance_code = base << 21;
        f_config_.acceptance_mask = (differ << 21) | 0x001FFFFF;
        f_config_.single_filter = true;

        // Reinstall driver with the updated filter
        twai_stop();
        twai_driver_uninstall();
        twai_driver_install(&g_config_, &t_config_, &f_config_);
        twai_start();
    }

    // ESP32 TWAI uses a FreeRTOS queue (32 deep) — twai_receive() with
    // timeout 0 is a cheap queue peek. Alert-based notification would
    // require a dedicated FreeRTOS task, which adds complexity for no
    // practical gain given the deep RX queue. Keep polling with HW filters.
    bool enableInterrupt(void (* /*onReady*/)()) override { return false; }

    bool read(CanFrame &frame) override
    {
        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK)
        {
            if (isBusOff())
                recover();
            return false;
        }
        frame.id = msg.identifier;
        frame.dlc = (msg.data_length_code <= 8) ? msg.data_length_code : 8;
        memset(frame.data, 0, 8);
        memcpy(frame.data, msg.data, frame.dlc);
        return true;
    }

    void send(const CanFrame &frame) override
    {
        twai_message_t msg = {};
        uint8_t dlc = (frame.dlc <= 8) ? frame.dlc : 8;
        msg.identifier = frame.id;
        msg.data_length_code = dlc;
        memcpy(msg.data, frame.data, dlc);

        if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK)
        {
            if (isBusOff())
                recover();
        }
    }

private:
    bool isBusOff()
    {
        twai_status_info_t status;
        if (twai_get_status_info(&status) != ESP_OK)
            return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    void recover()
    {
        twai_stop();
        twai_driver_uninstall();
        twai_driver_install(&g_config_, &t_config_, &f_config_);
        twai_start();
    }

    gpio_num_t txPin_;
    gpio_num_t rxPin_;
    twai_general_config_t g_config_;
    twai_timing_config_t t_config_;
    twai_filter_config_t f_config_;
};
