#include <Arduino.h>
#include "esp_camera.h"
#include "Camera.h"

/**
 * @brief Logging tag for http server
 */
static const char *TAG = "camera";

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

// Frame::Frame(const size_t &jpg_buf_len, uint8_t *jpg_buf)
// {
//     Serial.printf("Buffer length of frame is: %x", jpg_buf_len);
//     Serial.println();
//     Frame::_jpg_buf_len = _jpg_buf_len;
//     Frame::_jpg_buf = _jpg_buf;
// }

Camera::Camera(){
    Camera::fb = NULL;
    Camera::res = ESP_OK;
    Camera::_jpg_buf_len = 0;
    Camera::_jpg_buf = NULL;
    Camera::last_frame = 0;
}

Camera::~Camera(){
    if (Camera::fb)
    {
        Serial.println("clean frame buffer.");
        esp_camera_fb_return(Camera::fb);
        Camera::fb = NULL;
    }
    if(Camera::_jpg_buf){
        free(Camera::_jpg_buf);
    }
}

/**
 * @brief creates a config object, wich sets all the important, settings to find and interact with the camera
 * This method is private
 */
camera_config_t Camera::setupCameraPins()
{
    camera_config_t config;
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
    return config;
}
/**
 * @brief Inits the camera, so it's ready to use after this function
 */
void Camera::SetupCamera()
{
    Serial.print("Setup camera.");
    Serial.println();

    camera_config_t config = setupCameraPins();
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

/**
 * @brief finds out wich camera module is used and returns the path to the HTML UI for it
 * @return the path to the UI. If the camera can't be found,
 * an error is logged and null is the return
 */
char *Camera::GetCameraUi()
{
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL)
    {
        char *fileContent;
        if (s->id.PID == OV3660_PID)
        {
            fileContent = "/index_ov3660.html";
        }
        else if (s->id.PID == OV5640_PID)
        {
            fileContent = "/index_ov5640.html";
        }
        else
        {
            fileContent = "/index_ov2640.html";
        }
        return fileContent;
    }
    else
    {
        ESP_LOGE(TAG, "Camera sensor not found");
    }
    return NULL;
}

Frame &Camera::GetCameraStream()
{
    Frame frame = Frame();
    if (Camera::fb)
    {
        Serial.println("clean frame buffer.");
        esp_camera_fb_return(Camera::fb);
        Camera::fb = NULL;
        Camera::_jpg_buf = NULL;
    }
    if (!Camera::last_frame)
    {
        Camera::last_frame = esp_timer_get_time();
    }
    Serial.println("get frame.");
    Camera::fb = esp_camera_fb_get();
    if (!Camera::fb)
    {
        Serial.println("failed.");
        ESP_LOGE(TAG, "Camera capture failed");
        Camera::res = ESP_FAIL;
    }
    else
    {
        Camera::_timestamp.tv_sec = Camera::fb->timestamp.tv_sec;
        Camera::_timestamp.tv_usec = Camera::fb->timestamp.tv_usec;
        if (Camera::fb->format != PIXFORMAT_JPEG)
        {
            Serial.println("Try to convert to jpeg.");
            bool jpeg_converted = frame2jpg(Camera::fb, 80, Camera::_jpg_buf, Camera::_jpg_buf_len);
            esp_camera_fb_return(Camera::fb);
            Camera::fb = NULL;
            if (!jpeg_converted)
            {
                Serial.println("failed.");
                ESP_LOGE(TAG, "JPEG compression failed");
                Camera::res = ESP_FAIL;
            }
        }
        else
        {
            Serial.println("save frame.");
            frame._jpg_buf_len = Camera::fb->len;
            frame._jpg_buf = Camera::fb->buf;
        }
    }
    Serial.println("return frame.");
    return frame;
    // return test;
}
