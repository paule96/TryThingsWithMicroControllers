#include "esp_camera.h"
struct Frame
{
    // public:
    // Frame(const size_t &jpg_buf_len, uint8_t *jpg_buf);
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
};

class Camera
{
public:
    Camera();
    ~Camera();
    void SetupCamera();
    char *GetCameraUi();
    Frame GetCameraStream();
    struct timeval _timestamp;
private:
    camera_config_t setupCameraPins();
    camera_fb_t *fb;

    esp_err_t res;
    size_t *_jpg_buf_len;
    uint8_t **_jpg_buf;
    int64_t last_frame;
};
