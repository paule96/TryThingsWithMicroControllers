#include <Arduino.h>
#include "esp_camera.h"
#include "WiFi.h"
#include "http_server.h"
#include "SD.h"
#include "SPI.h"

/**
 * @brief the following region descripes all the needed pins for the camera module.
 */
#pragma region camera_pins
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13
#pragma endregion camera_pins

#pragma region wlan_config
// TODO: should be somehow typed in by the user.
//  That would be much safer. And the device should forget about it
//  after a reboot boot.
const char *ssid = "Zuhause ist werbefreie zone";
const char *password = "hier koennte ihre werbung stehen";
#pragma endregion wlan_config

camera_config_t config;

/**
 * @brief  the http server for the static content
 */
httpd_handle_t camera_httpd = NULL;
/**
 * @brief  the http server for streaming the video feed
 */
httpd_handle_t stream_httpd = NULL;

/**
 * @brief Logging tag for http server
 */
static const char *TAG = "camera_httpd";

char *readFile(char *path)
{
  File myFile = SD.open(path, FILE_READ);
  if (!myFile)
  {
    Serial.printf("error opening %x", path);
    Serial.println();
  }
  else
  {
    size_t size = myFile.size();
    char charResult[size];
    String result;
    while (myFile.available())
    {
      result = myFile.readString();
    }
    result.toCharArray(charResult, sizeof(charResult));
    return charResult;
  }
  char* test;
  return test;
}

void setupCameraPins()
{
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // if PSRAM IC present,init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated framebuffer.
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
}

void setupCamera()
{
  setupCameraPins();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  // optimize now the output of the sensor a bit
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);       // inital configuration is, that the camera is flipped. We flip it back here
  s->set_brightness(s, 1);  // initial configuration is to dark
  s->set_saturation(s, -1); // initial configuration is too saturated. so we reduce it.
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

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>Hello world</h1><body></html>";
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL)
  {
    char *fileContent;
    if (s->id.PID == OV3660_PID)
    {
      fileContent = readFile("index_ov3660.html");
    }
    else if (s->id.PID == OV5640_PID)
    {
      fileContent = readFile("index_ov5640.html");
    }
    else
    {
      fileContent = readFile("index_ov2640.html");
    }
    return httpd_resp_send(req, fileContent, sizeof(fileContent));
  }
  else
  {
    ESP_LOGE(TAG, "Camera sensor not found");
    return httpd_resp_send_500(req);
  }
}

static esp_err_t status_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>status</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>control</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t capture_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>capture</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>stream</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t bmp_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>bmp</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t xclk_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>xclk</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t reg_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>reg</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t greg_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>greg</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t pll_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>pll</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

static esp_err_t win_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  const char *response = "<!DOCTYPE html><html><body><h1>win</h1><body></html>";
  return httpd_resp_send(req, response, sizeof(response));
}

void startCameraServer()
{
  httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();
  httpConfig.max_uri_handlers = 16;
  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL};
  httpd_uri_t status_uri = {
      .uri = "/status",
      .method = HTTP_GET,
      .handler = status_handler,
      .user_ctx = NULL};
  httpd_uri_t cmd_uri = {
      .uri = "/control",
      .method = HTTP_GET,
      .handler = cmd_handler,
      .user_ctx = NULL};
  httpd_uri_t capture_uri = {
      .uri = "/capture",
      .method = HTTP_GET,
      .handler = capture_handler,
      .user_ctx = NULL};

  httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  httpd_uri_t bmp_uri = {
      .uri = "/bmp",
      .method = HTTP_GET,
      .handler = bmp_handler,
      .user_ctx = NULL};

  httpd_uri_t xclk_uri = {
      .uri = "/xclk",
      .method = HTTP_GET,
      .handler = xclk_handler,
      .user_ctx = NULL};

  httpd_uri_t reg_uri = {
      .uri = "/reg",
      .method = HTTP_GET,
      .handler = reg_handler,
      .user_ctx = NULL};

  httpd_uri_t greg_uri = {
      .uri = "/greg",
      .method = HTTP_GET,
      .handler = greg_handler,
      .user_ctx = NULL};

  httpd_uri_t pll_uri = {
      .uri = "/pll",
      .method = HTTP_GET,
      .handler = pll_handler,
      .user_ctx = NULL};

  httpd_uri_t win_uri = {
      .uri = "/resolution",
      .method = HTTP_GET,
      .handler = win_handler,
      .user_ctx = NULL};

  ESP_LOGI(TAG, "Starting web server on port: '%d'", httpConfig.server_port);
  if (httpd_start(&camera_httpd, &httpConfig) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &bmp_uri);

    httpd_register_uri_handler(camera_httpd, &xclk_uri);
    httpd_register_uri_handler(camera_httpd, &reg_uri);
    httpd_register_uri_handler(camera_httpd, &greg_uri);
    httpd_register_uri_handler(camera_httpd, &pll_uri);
    httpd_register_uri_handler(camera_httpd, &win_uri);
  }

  httpConfig.server_port += 1;
  httpConfig.ctrl_port += 1;
  ESP_LOGI(TAG, "Starting stream server on port: '%d'", httpConfig.server_port);
  if (httpd_start(&stream_httpd, &httpConfig) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("init SD");
  if (!SD.begin(5))
  {
    Serial.println("initialization failed!");
    while (1)
      ;
  }

  Serial.print("Setup camera.");
  Serial.println();

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
  // TODO: start the camera server here

  Serial.printf("Camera ready! Use 'http://%x to connect", WiFi.localIP());
  Serial.println();
}

void loop()
{
  // the main loop is the camera server. So we just set this CPU task to delay as long as he can.
  delay(100000);
}

