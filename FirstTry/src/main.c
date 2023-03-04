#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/ledc.h"

void app_main() {
    printf("hello");

    // setup led stuff
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = (1),
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = ledc_timer.speed_mode,
        .timer_sel = ledc_timer.timer_num,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = (5),
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(
        ledc_timer.speed_mode,
        ledc_channel.channel,
        (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
    ));
    ESP_ERROR_CHECK(ledc_update_duty(ledc_timer.speed_mode, ledc_channel.channel));


}