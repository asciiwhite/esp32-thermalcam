#include "infobar.h"
#include "battery_voltage.h"

#include <TFT_eSPI.h>

InfoBar::InfoBar(TFT_eSPI& _tft)
 : tft(_tft)
{}

void InfoBar::update(uint32_t timeInMillis, uint32_t processingTime, uint32_t frameTime) {
  if (timeInMillis > uiNextRefreshInMillis)
  {
    uiNextRefreshInMillis += uiRefreshRateInMillis;

    tft.setTextSize(1);
    tft.setTextFont(1);
    
    printBatteryVoltage();
    printRunTime();
    printFrameTime(processingTime, frameTime);
  }
}

void InfoBar::printFrameTime(uint32_t processingTime, uint32_t frameTime) {
  tft.setCursor(0, 0);
  tft.print(frameTime);
  tft.print("/");
  tft.print(processingTime);
  tft.print("/");
  tft.print(frameTime - processingTime);
  tft.print("ms   ");
}

void InfoBar::printRunTime() {
  tft.setCursor(tft.width() / 2 - runTimeWidth, 0);
  const uint32_t time = millis();
  const uint32_t seconds = (time / 1000) % 60;
  const uint32_t minutes = (time / 60000) % 60;
  const uint32_t hours   = time / 3600000;

  char tbs[16];
  sprintf(tbs, "%02d:%02d:%02d", hours, minutes, seconds);
  tft.print(tbs);
}

void InfoBar::printBatteryVoltage() {
  tft.setCursor(tft.width() - batteryVoltageWidth, 0);
  tft.print(getBatteryVoltage(), 2);
  tft.println("V");
}
