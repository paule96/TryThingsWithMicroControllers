#include <Arduino.h>
#include "esp_camera.h"
#include "Camera.h"
#include <vector>
#include "color_detector.hpp"
#include "dl_image.hpp"
#include "dl_variable.hpp"
#include "fb_gfx.h"


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

ColorDetector detector;

/**
 * @brief inits some variables for the camera class
 */
Camera::Camera()
{
    Camera::fb = NULL;
    Camera::res = ESP_OK;
    Camera::_jpg_buf_len = 0;
    Camera::_jpg_buf = NULL;
    Camera::last_frame = 0;
    // expectation this line do nothing
    detector.set_detection_shape({10, 10, 1});
    // this regisers the color. We don't know what the last three numbers are
    // detector.register_color({colorToDetect[0], colorToDetect[1], colorToDetect[2], 255, 90, 255});
    // std::vector<uint8_t> lightred = {213, 99, 113};
    // std::vector<uint8_t> darkred = {59, 27, 31};
    std::vector<color_info_t> std_color_info = {
                                                    {{156, 10, 70, 255, 90, 255}, 64, "red"}
                                                    // {{11, 22, 70, 255, 90, 255}, 64, "orange"},
                                                    // {{23, 33, 70, 255, 90, 255}, 64, "yellow"},
                                                    // {{34, 75, 70, 255, 90, 255}, 64, "green"},
                                                    // {{76, 96, 70, 255, 90, 255}, 64, "cyan"},
                                                    // {{97, 124, 70, 255, 90, 255}, 64, "blue"},
                                                    // {{125, 155, 70, 255, 90, 255}, 64, "purple"},
                                                    // {{0, 180, 0, 40, 220, 255}, 64, "white"},
                                                    // {{0, 180, 0, 50, 50, 219}, 64, "gray"},
                                                    // {{0, 180, 0, 255, 0, 45}, 64, "black"}
                                                };
    for (size_t i = 0; i < std_color_info.size(); i++)
    {
        detector.register_color(std_color_info[i].color_thresh, std_color_info[i].area_thresh, std_color_info[i].name);
    }
}

/// @brief clean all the used buffers
Camera::~Camera()
{
    if (Camera::fb)
    {
        Serial.println("clean frame buffer.");
        esp_camera_fb_return(Camera::fb);
        Camera::fb = NULL;
    }
    if (Camera::_jpg_buf)
    {
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
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
        config.jpeg_quality = 1;
        config.fb_count = 2;
    }
    else
    {
        // Limit the frame size when PSRAM is not available
        config.frame_size = FRAMESIZE_SVGA;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_DRAM;
        // 3 is the best possible value
        config.jpeg_quality = 3;
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
    config.frame_size = FRAMESIZE_240X240;
    config.pixel_format = PIXFORMAT_RGB565;
    config.jpeg_quality = 10;
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
    // optimize now the output of the sensor a bit
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 0);       // inital configuration is, that the camera is flipped. We flip it back here
    s->set_brightness(s, 1);  // initial configuration is to dark
    s->set_saturation(s, -1); // initial configuration is too saturated. so we reduce it.
}

/**
 * @brief Get's the camera sensor that is installed
 * @return returns the type if the sensor
 */
Sensor Camera::GetSensor()
{
    sensor_t *s = esp_camera_sensor_get();
    Sensor sensor = Sensor();
    if (s == nullptr)
    {
        ESP_LOGE(TAG, "Camera sensor not found");
        return sensor;
    }
    // camera_pid_t result = static_cast<camera_pid_t>(s->id.PID);

    sensor.type = static_cast<camera_pid_t>(s->id.PID);
    return sensor;
}

/// @brief Resets a frame object and free the memory for it
/// @param frame The frame that should be resetted
void Camera::ResetFrame(Frame frame)
{
    frame.buffer = NULL;
    frame.length = 0;
    frame.index = 0;
}

/// @brief If the method get's called it returns a single frame as jpg.
/// That can be used for a stream like behavior
/// @return A single frame, as jpg
Frame Camera::GetCameraStream()
{
    Frame frame = Frame();
    if (Camera::fb)
    {
        // Serial.println("clean frame buffer.");
        esp_camera_fb_return(Camera::fb);
        Camera::fb = NULL;
        Camera::_jpg_buf = NULL;
    }
    if (!Camera::last_frame)
    {
        Camera::last_frame = esp_timer_get_time();
    }
    // Serial.println("get frame.");
    Camera::fb = esp_camera_fb_get();
    if (!Camera::fb)
    {
        Serial.println("failed.");
        ESP_LOGE(TAG, "Camera capture failed");
        Camera::res = ESP_FAIL;
    }
    else
    {
        frame.timestamp.tv_sec = Camera::fb->timestamp.tv_sec;
        frame.timestamp.tv_usec = Camera::fb->timestamp.tv_usec;
        // if (Camera::fb->format != PIXFORMAT_JPEG)
        // {
        //     ESP_LOGI(TAG, "Try to convert to jpeg.");
        //     uint8_t** test;
        //     bool jpeg_converted = frame2jpg(Camera::fb, 80, test, Camera::_jpg_buf_len);
        //     Camera::_jpg_buf = test[0];
        //     esp_camera_fb_return(Camera::fb);
        //     Camera::fb = NULL;
        //     if (!jpeg_converted)
        //     {
        //         Serial.println("failed.");
        //         ESP_LOGE(TAG, "JPEG compression failed");
        //         Camera::res = ESP_FAIL;
        //     }
        // }
        // else{
        //     ESP_LOGI(TAG, "It's jpeg, use a new buffer");
        //     frame.length = Camera::fb->len;
        //     frame.height = Camera::fb->height;
        //     frame.width = Camera::fb->width;
        //     Camera::_jpg_buf = std::move(Camera::fb->buf);
        //     esp_camera_fb_return(Camera::fb);
        //     Camera::fb = NULL;
        //     ESP_LOGI(TAG, "jpeg copy done");
        // }
        // Serial.println("save frame.");
        frame.length = Camera::fb->len;
        frame.buffer = Camera::fb->buf;
        frame.height = Camera::fb->height;
        frame.width = Camera::fb->width;
        frame.index = 0;
    }
    // Serial.println("return frame.");
    return frame;
}

static color_detection_state_t gEvent = COLOR_DETECTION_IDLE;
static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueEvent = NULL;
static QueueHandle_t xQueueFrameO = NULL;
static QueueHandle_t xQueueResult = NULL;
static SemaphoreHandle_t xMutex;


Frame Camera::GetCameraStream(std::vector<uint8_t> colorToDetect)
{
    if (colorToDetect.size() != 3)
    {
        ESP_LOGE(TAG, "The color to detect must be a vector of 3 integers. (RGB)");
        throw "Need RGB vector to detect colors";
    }
    Frame frame = GetCameraStream();

    // some local event
    color_detection_state_t _gEvent;
    std::vector<uint8_t> color_thresh;
    ESP_LOGI(TAG, "Try to start image detection now");
    ESP_LOGI(TAG, "analyze image now");
    std::vector<std::vector<color_detect_result_t>> &results = detector.detect((uint16_t *)frame.buffer, {(int)frame.height, (int)frame.width, 3});
    // detector.print_detection_results();
    ESP_LOGI(TAG, "image analyzed now");
    if (results.size() > 0)
    {
        std::vector<color_info_t> colors = detector.get_registered_colors();
        // ESP_LOGI(TAG, "Could detect the color %u, %u, %u", colorToDetect[0], colorToDetect[1], colorToDetect[2]);
        for (size_t i = 0; i < results.size(); i++)
        {
            ESP_LOGI(TAG, "now print locations for: %s", colors[i].name);
            for (size_t j = 0; j < results[i].size(); j++)
            {
                ESP_LOGI(TAG, "Location %u, %u", results[i][j].center[0], results[i][j].center[1]);
            }
        }

        // ESP_LOGI(TAG, "Draw results");
        // Tensor<uint8_t> aligned_face_rgb888_316_2;
        // aligned_face_rgb888_316_2.set_element((uint8_t *)aligned_face112_image_bgr_316_2).set_shape({112, 112, 3}).set_auto_free(false);

        // set the Tensor of test image.
        // Tensor<uint8_t> rgby;
        // convert_pixel_rgb565_to_rgb888(frame.buffer,)

        // detector.draw_segmentation_results(frame.)
        ESP_LOGI(TAG, "Start drawing results");
        detector.draw_segmentation_results((uint16_t *)frame.buffer, {(int)frame.height, (int)frame.width}, {0x00F8}, false);
        ESP_LOGI(TAG, "Finish drawing results");
    }
    else
    {
        ESP_LOGI(TAG, "Couldn't detect the color %u, %u, %u", colorToDetect[0], colorToDetect[1], colorToDetect[2]);
    }
    ESP_LOGI(TAG, "convert frame to jpeg %u, %u with a lenght of %u", frame.width, frame.height, frame.length);
    uint8_t* test;
    fmt2jpg(frame.buffer, frame.length, frame.width, frame.height, PIXFORMAT_RGB565, 10, &test, &frame.length);
    // bool jpeg_converted = frame2jpg(frame.buffer, 80, test, frame.length);
    ESP_LOGI(TAG, "now free up buffers.");
    // if(frame.buffer){
    //     free(frame.buffer);
    //     frame.buffer = NULL;
    // }
    ESP_LOGI(TAG, "write new buffer");
    frame.buffer = test;
    return frame;
}