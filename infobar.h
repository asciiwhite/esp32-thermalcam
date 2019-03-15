#ifndef H_INFOBAR
#define H_INFOBAR

#include <Arduino.h>

class TFT_eSPI;

class InfoBar
{
  public:
    InfoBar(TFT_eSPI& tft);
    
    void update(uint32_t timeInMillis, uint32_t processingTime, uint32_t frameTime);

  private:
    TFT_eSPI& tft;

    const uint32_t batteryVoltageWidth  = 30;
    const uint32_t runTimeWidth  = 20;

    const uint32_t uiRefreshRateInMillis = 500;
    uint32_t uiNextRefreshInMillis       = 0;

    void printFrameTime(uint32_t processingTime, uint32_t frameTime);
    void printRunTime();
    void printBatteryVoltage();
};

#endif
