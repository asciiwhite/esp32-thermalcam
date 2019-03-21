#include "infobar.h"
#include "mlxcamera.h"

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

MLXCamera camera(tft);
InfoBar infoBar = InfoBar(tft);
const uint32_t InfoBarHeight = 10;
const uint32_t MaxFrameTimeInMillis = 33;

void setup() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    Serial.begin(115200);
    while(!Serial);

    if (!camera.init())
    {      
      tft.setCursor(75, tft.height() / 2);
      tft.print("No camera detected!");
      vTaskDelete(NULL); // remove loop task
    }
}

void loop() {
    const long start = millis();

    camera.readImage();

    const long processingTime = millis() - start;

    tft.setCursor(0, InfoBarHeight);
    camera.drawImage(7);
    camera.drawLegend();
    camera.drawCenterMeasurement();

    const long frameTime = millis() - start;

    Serial.printf("Draw: %d\n", frameTime - processingTime);

    infoBar.update(start, processingTime, frameTime);
    if (frameTime < MaxFrameTimeInMillis)
    delay(MaxFrameTimeInMillis - frameTime);
}
