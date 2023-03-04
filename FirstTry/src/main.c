#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/ledc.h"
const int maxBrithness = 8190;
const int minBrithness = 0;
void updateLed(int brithness, ledc_timer_config_t timer, ledc_channel_config_t channel){
    if(brithness > maxBrithness){
        //TODO: fehler
    }
    if(brithness < minBrithness){
        //TODO: fehler
    }
    ESP_ERROR_CHECK(ledc_set_duty(
            timer.speed_mode,
            channel.channel,
            brithness // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
        ));
    ESP_ERROR_CHECK(ledc_update_duty(timer.speed_mode, channel.channel));

}

void app_main() {
    printf("hello");

    // setup led stuff
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = ledc_timer.speed_mode,
        .timer_sel = ledc_timer.timer_num,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 2,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    // TickType_t delay = 500/portTICK_PERIOD_MS;for(;;);

    while(true){
        updateLed(8190, ledc_timer, ledc_channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        updateLed(6195, ledc_timer, ledc_channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        updateLed(4095, ledc_timer, ledc_channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        updateLed(2050, ledc_timer, ledc_channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        updateLed(0, ledc_timer, ledc_channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}