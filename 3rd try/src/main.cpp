#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "FilesOnSdCard.h"
#include "Camera.h"
#include "WebServer.h"
#include "esp_log.h"
#include "esp32-hal-log.h"

#pragma region wlan_config
// TODO: should be somehow typed in by the user.
//  That would be much safer. And the device should forget about it
//  after a reboot boot.
const char *ssid = "Zuhause ist werbefreie zone";
const char *password = "hier koennte ihre werbung stehen";
#pragma endregion wlan_config

/**
 * @brief Logging tag for http server
 */
static const char *TAG = "main";

void disableAllLeds()
{
  ESP_LOGI(TAG, "try to disable leds");
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_13_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 5000,
      .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .gpio_num = 2,
      .speed_mode = ledc_timer.speed_mode,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = ledc_timer.timer_num,
      .duty = 0,
      .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  ESP_ERROR_CHECK(ledc_set_duty(
      ledc_timer.speed_mode,
      ledc_channel.channel,
      0 // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
      ));
  ESP_ERROR_CHECK(ledc_update_duty(ledc_timer.speed_mode, ledc_channel.channel));
}

String getIpV4Address(const IPAddress &ipAddress)
{
  return String(ipAddress[0] + String(".") +
                ipAddress[1] + String(".") +
                ipAddress[2] + String(".") +
                ipAddress[3] + String("."));
}

void setupWifi()
{
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("wifi connected");

  // TODO: find out how we can show IPV4 and if it's there then IPV6 information
  //  IPAddress localV4Ip = WiFi.localIP();
  //  IPv6Address localV6Ip = WiFi.localIPv6();
}

void setup()
{
  // esp_log_set_vprintf(esp_apptrace_vprintf);
  // esp_log_set_level_master(ESP_LOG_INFO);
  esp_log_level_set("Verbose ", ESP_LOG_VERBOSE);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  InitSdCard();
  Serial.print("try to connect to wifi.");
  Serial.println();
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("wifi connected");

  StartCameraServer();
  Serial.print("Camera ready! Use 'http://"+ getIpV4Address(WiFi.localIP()) + " to connect");
  Serial.println();
}

void loop()
{
  // disableAllLeds();
  // the main loop is the camera server. So we just set this CPU task to delay as long as he can.
  delay(100000);
}
