void SetupCamera();
char* GetCameraUi();
class Frame
{
public:
    Frame(size_t jpg_buf_len, uint8_t *jpg_buf);
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
};
Frame GetCameraStream();

