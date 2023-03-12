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
    size_t size = 0;

    // always remember: memcpy don't moves the cursor, so this is always a two liner
    if(index){
      Serial.println("Write boundry");
      Serial.println(_STREAM_BOUNDARY);
      size_t blen = strlen(_STREAM_BOUNDARY);
      memcpy(buffer, _STREAM_BOUNDARY, blen);
      size += blen;
      buffer += blen;
    }

    Serial.println("Write part header");
    Serial.printf(_STREAM_PART, frame.length, frame.timestamp.tv_sec, frame.timestamp.tv_usec);
    size_t hlen = sprintf((char *)buffer, _STREAM_PART, frame.length, frame.timestamp.tv_sec, frame.timestamp.tv_usec);
    buffer += hlen;
    size += hlen;

    // get the max lenght minus headers
    size_t maxBodyLenght = maxLen - size;
    if(maxBodyLenght > frame.length){
      size = maxBodyLenght - frame.length;
    }
    Serial.println("Write jpeg");
    memcpy(buffer, frame.buffer,frame.length);
    // the buffer cursor shouldn't be moved here
    Serial.printf("finish response with a size of: %x", size);
    Serial.println();
    return size; });
  // disable CORS
  // TODO: check if we really need it. It should be okay without
  response->addHeader("Access-Control-Allow-Origin", "*");
  // a header to define the max framerate
  response->addHeader("X-Framerate", "60");
  Serial.println("Now start sending the request.");
  // answer the request
  request->send(response);
}
/// @brief Responde to a request with a single image
/// @param request The request, that will responded with an image
void SingleImage(AsyncWebServerRequest *request){

  request->send("image/jpeg", 100000000, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t{
    Frame frame = camera.GetCameraStream();
    Serial.printf("Frame length: %x", frame.length);
    Serial.println();
    memcpy(buffer, frame.buffer, frame.length);
    return frame.length;
  });
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
  // adds a handler for sending the camera stream
  server.on("/image", HTTP_GET, SingleImage);
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
