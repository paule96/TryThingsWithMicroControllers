#include "ESPAsyncWebServer.h"
#include "FilesOnSdCard.h"
#include "Camera.h"
FS& filesystem = GetCurrentMount();

/**
 * @brief Logging tag for http server
 */
static const char *TAG = "camera_httpd";

/**
 * @brief starts a webserver to serve the camerui
*/
void StartCameraServer()
{
    ESP_LOGI(TAG, "Starting web server on port: '80'");
    AsyncWebServer server(80);
    filesystem = GetCurrentMount();
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                request->send(filesystem, GetCameraUi(), "text/html"); });
    server.begin();
    ESP_LOGI(TAG, "Starting stream server on port: '80'");
}