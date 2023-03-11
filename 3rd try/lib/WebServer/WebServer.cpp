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
Camera camera;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

/// @brief makes it possible returns a stream of the camera
/// @param request the request object, that will get an answer
void StreamCameraFeed(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginChunkedResponse(_STREAM_CONTENT_TYPE, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                                                   {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  Serial.printf("Starting stream frame. Max length: %x, ", maxLen);
    Frame frame = camera.GetCameraStream();
    Serial.printf("Frame length: %x", frame._jpg_buf_len);
    Serial.println();
    char* part_buf[128];
    size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, frame._jpg_buf_len, camera._timestamp.tv_sec, camera._timestamp.tv_usec);
    size_t maxBufferRead;
    size_t size = frame._jpg_buf_len + hlen + strlen(_STREAM_BOUNDARY);
    Serial.println(_STREAM_BOUNDARY);
    Serial.println(_STREAM_PART);
    Serial.printf(_STREAM_PART, frame._jpg_buf_len, camera._timestamp.tv_sec, camera._timestamp.tv_usec);
    if(maxLen > size){
      maxBufferRead = size;
    }else{
      maxBufferRead = maxLen;
    }

    Serial.println("Write boundry");
    memcpy(buffer, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    buffer += strlen(_STREAM_BOUNDARY);
    Serial.println("Write part header");
    memcpy(buffer, &part_buf, hlen);
    buffer += hlen;
    Serial.println("Write jpeg");
    memcpy(buffer, &frame._jpg_buf,frame._jpg_buf_len);
    buffer += frame._jpg_buf_len;
    Serial.println("finish response");
    return maxBufferRead; });
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("X-Framerate", "60");

  request->send(response);
}

/**
 * @brief starts a webserver to serve the camerui
 */
void StartCameraServer()
{
  camera.SetupCamera();
  ESP_LOGI(TAG, "Starting web server on port: '80'");

  filesystem = GetCurrentMount();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(filesystem, camera.GetCameraUi(), "text/html"); });
  server.on("/stream", HTTP_GET, StreamCameraFeed);
  server.begin();
  ESP_LOGI(TAG, "Starting stream server on port: '80'");
}

void StopCameraServer()
{
  camera.~Camera();
  server.~AsyncWebServer();
}
