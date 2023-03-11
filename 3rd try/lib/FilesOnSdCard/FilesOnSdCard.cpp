#include <Arduino.h>
#include "SD_MMC.h"

/**
 * @brief Init the SD card functions and mounts it to the path '/sdcard'
 * It also shows how big the connected SD card is.
 * @return is true when the SD card is okay. false if it can't init
 * or there is no SD card in the slot
*/
bool InitSdCard()
{
    Serial.println("init SD");
    SD_MMC.setPins(39, 38, 40);
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5))
    {
        Serial.println("initialization failed!");
        return false;
    }
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("No SD_MMC card attached");
        return false;
    }
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
    return true;
}

/**
 * @brief Reads a file from the SD card.
 * @return Returns the file object from the SD card.
 * If the file object is empty, the file couldn't be open
*/
File ReadFile(char* path)
{
  File myFile = SD_MMC.open(path, FILE_READ);
  if (!myFile)
  {
    Serial.printf("error opening %x", path);
    Serial.println();
  }
  else
  {
    return myFile;
  }
  return File();
}

/**
 * @brief returns the configured filesystem
*/
FS& GetCurrentMount(){
  return SD_MMC;
}