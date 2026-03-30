#pragma once

#include "can_driver.h"
#include <driver/twai.h>

class TWAIDriver : public CanDriver {
public:
    TWAIDriver(gpio_num_t txPin, gpio_num_t rxPin)
        : txPin_(txPin), rxPin_(rxPin) {}

    bool init() override {
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(txPin_, rxPin_, TWAI_MODE_NORMAL);
        g_config.rx_queue_len = 32;
        g_config.tx_queue_len = 16;

        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
        if (twai_start() != ESP_OK) return false;
        return true;
    }

    bool read(CanFrame& frame) override {
        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK) {
            if (isBusOff()) recover();
            return false;
        }
        frame.id  = msg.identifier;
        frame.dlc = msg.data_length_code;
        memcpy(frame.data, msg.data, msg.data_length_code);
        return true;
    }

    void send(const CanFrame& frame) override {
        twai_message_t msg = {};
        msg.identifier = frame.id;
        msg.data_length_code = frame.dlc;
        memcpy(msg.data, frame.data, frame.dlc);

        if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
            if (isBusOff()) recover();
        }
    }

private:
    bool isBusOff() {
        twai_status_info_t status;
        if (twai_get_status_info(&status) != ESP_OK) return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    void recover() {
        twai_stop();
        twai_driver_uninstall();
        init();
    }

    gpio_num_t txPin_;
    gpio_num_t rxPin_;
};
