#include "esp_camera.h"
struct Frame
{
    /// @brief the length of the buffer
    size_t length = 0;
    /// @brief The index where we should read the next values from the buffer
    size_t index = 0;
    /// @brief buffer for the jpg frame
    uint8_t *buffer = NULL;
    /// @brief the timestamp of the frame
    timeval timestamp;
};

struct Sensor{
    /// @brief can be empty type. Please check just for known types
    camera_pid_t type;
};

class Camera
{
public:
    Camera();
    ~Camera();
    void SetupCamera();
    Sensor GetSensor();
    Frame GetCameraStream();
    void ResetFrame(Frame frame);
private:
    camera_config_t setupCameraPins();
    camera_fb_t *fb;
    esp_err_t res;
    size_t *_jpg_buf_len;
    uint8_t **_jpg_buf;
    int64_t last_frame;
};

