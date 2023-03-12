#include "Arduino.h"

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Total heap: %d", ESP.getHeapSize());
    Serial.println();
    Serial.printf("Free heap: %d", ESP.getFreeHeap());
    Serial.println();
    Serial.printf("Total PSRAM: %d", ESP.getPsramSize());
    Serial.println();
    Serial.printf("Free PSRAM: %d", ESP.getFreePsram());
    Serial.println();
}

void loop()
{
    delay(1000);
}