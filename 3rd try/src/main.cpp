#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "FilesOnSdCard.h"
#include "Camera.h"
#include "WebServer.h"

#pragma region wlan_config
// TODO: should be somehow typed in by the user.
//  That would be much safer. And the device should forget about it
//  after a reboot boot.
const char *ssid = "Zuhause ist werbefreie zone";
const char *password = "hier koennte ihre werbung stehen";
#pragma endregion wlan_config

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
  Serial.printf("Camera ready! Use 'http://%x to connect", WiFi.localIP().toString());
  Serial.println();
}

void loop()
{
  // the main loop is the camera server. So we just set this CPU task to delay as long as he can.
  delay(100000);
}
