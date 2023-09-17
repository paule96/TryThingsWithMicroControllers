#include "SD_MMC.h"

bool InitSdCard();
File ReadFile(char* path);
FS& GetCurrentMount();