#include "ESPAsyncWebServer.h"
#include "FilesOnSdCard.h"
#include "Camera.h"
FS &filesystem = GetCurrentMount();

/**
 * @brief Logging tag for http server
 */
static const char *TAG = "camera_httpd";

/// @brief The webserver that will be hosted to communicate with the camera
AsyncWebServer server(80);

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

/// @brief makes it possible returns a stream of the camera
/// @param request the request object, that will get an answer
void StreamCameraFeed(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginChunkedResponse(_STREAM_CONTENT_TYPE, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
    Frame frame = GetCameraStream();
    size_t maxBufferRead;
    if(maxLen > frame._jpg_buf_len){
      maxBufferRead = frame._jpg_buf_len;
    }else{
      maxBufferRead = maxLen;
    }
    for (size_t i = 0; i < maxBufferRead; i++)
    {
      buffer[i] = frame._jpg_buf[i];
    }
    return GetCameraStream()._jpg_buf_len;
  });
}

/**
 * @brief starts a webserver to serve the camerui
 */
void StartCameraServer()
{
  ESP_LOGI(TAG, "Starting web server on port: '80'");

  filesystem = GetCurrentMount();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(filesystem, GetCameraUi(), "text/html"); });
  server.on("/stream", HTTP_GET, StreamCameraFeed);
  server.begin();
  ESP_LOGI(TAG, "Starting stream server on port: '80'");
}

