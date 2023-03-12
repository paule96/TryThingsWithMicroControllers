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
/// @brief The camera class object that will be use.
// this line calls the constratrutor
Camera camera;

/**
 * @brief This will define a section in a multipart HTTP request.
 * This webserver will only use one section in the mulitpart HTTP request.
*/
#define PART_BOUNDARY "123456789000000000000987654321"
/**
 * @brief The content type of the stream response
*/
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
/**
 * @brief marks the start of a single part of a multipart HTTP request
*/
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
/**
 * @brief The headers of a single part of a multipart HTTP request;
 * Never forget to format this string.
*/
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
    Serial.printf("Frame length: %x", frame.length);
    Serial.println();
    char* part_buf[128];
    size_t maxBufferRead;
    size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, frame.length, frame.timestamp.tv_sec, frame.timestamp.tv_usec);
    size_t size = frame.length + hlen + strlen(_STREAM_BOUNDARY);
    Serial.println(_STREAM_BOUNDARY);
    Serial.println(_STREAM_PART);
    Serial.printf(_STREAM_PART, frame.length, frame.timestamp.tv_sec, frame.timestamp.tv_usec);
    if(maxLen > size){
      maxBufferRead = size;
    }else{
      maxBufferRead = maxLen;
    }

    Serial.println("Write boundry");
    // always remember: memcpy don't moves the cursor, so this is always a two liner
    memcpy(buffer, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    buffer += strlen(_STREAM_BOUNDARY);
    Serial.println("Write part header");
    memcpy(buffer, &part_buf, hlen);
    buffer += hlen;
    Serial.println("Write jpeg");
    memcpy(buffer, &frame.buffer,frame.length);
    buffer += frame.length;
    Serial.println("finish response");
    return maxBufferRead; });
  // disable CORS
  // TODO: check if we really need it. It should be okay without
  response->addHeader("Access-Control-Allow-Origin", "*");
  // a header to define the max framerate
  response->addHeader("X-Framerate", "60");
  // answer the request
  request->send(response);
}

/**
 * @brief starts a webserver to serve the camerui and the stream
 */
void StartCameraServer()
{
  // before the server can start, the camera needs to be ready
  camera.SetupCamera();
  ESP_LOGI(TAG, "Starting web server on port: '80'");
  filesystem = GetCurrentMount();
  // adds a handler for the index of the webserver
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(filesystem, camera.GetCameraUi(), "text/html"); });
  // adds a handler for sending the camera stream
  server.on("/stream", HTTP_GET, StreamCameraFeed);
  // starts the server
  server.begin();
  ESP_LOGI(TAG, "Starting stream server on port: '80'");
}

/**
 * @brief This will stop the webserver that provide the stream and the camera.
*/
void StopCameraServer()
{
  // first dispose the camera
  camera.~Camera();
  // then the server
  server.~AsyncWebServer();
}
