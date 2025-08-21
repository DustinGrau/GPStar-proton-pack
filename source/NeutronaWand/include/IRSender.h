#pragma once
#include <Arduino.h>
#include "driver/rmt.h"

// IRSender.h
class IRSender {
public:
    // Accept int for pin and cast internally
    explicit IRSender(int pin)
        : _pin(static_cast<gpio_num_t>(pin))
    {}

    void begin() {
        // Configure RMT TX
        rmt_config_t config;
        config.rmt_mode = RMT_MODE_TX;
        config.channel = RMT_CHANNEL_2;          // fixed channel 1
        config.gpio_num = _pin;
        config.mem_block_num = 1;
        config.clk_div = 80;                     // 1 MHz RMT clock (80MHz/80)
        config.tx_config.loop_en = false;
        config.tx_config.carrier_en = true;
        config.tx_config.carrier_freq_hz = 38000;   // 38 kHz carrier
        config.tx_config.carrier_duty_percent = 50; // 50% duty cycle
        config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
        config.tx_config.idle_output_en = true;
        config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        rmt_config(&config);
        rmt_driver_install(config.channel, 0, 0);
    }

    void sendRaw(const uint16_t *data, size_t length) {
        rmt_item32_t items[length/2 + 1]; // ensure enough items
        for (size_t i = 0; i < length; i+=2) {
            items[i/2].level0 = 1;
            items[i/2].duration0 = data[i];
            items[i/2].level1 = 0;
            items[i/2].duration1 = (i+1 < length) ? data[i+1] : 0;
        }
        rmt_write_items(RMT_CHANNEL_2, items, (length + 1)/2, false);
        rmt_wait_tx_done(RMT_CHANNEL_2, portMAX_DELAY);
    }

private:
    gpio_num_t _pin;
};
