#include "infobar.h"
#include "mlxcamera.h"

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

MLXCamera camera(tft);
InfoBar infoBar = InfoBar(tft);
const uint32_t InfoBarHeight = 10;
const uint32_t MaxFrameTimeInMillis = 33;

InterpolationType interpolationType = InterpolationType::eLinear;

void setup() {
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Serial.begin(115200);
    while(!Serial);

    if (!camera.init())
    {      
      tft.setCursor(75, tft.height() / 2);
      tft.print("No camera detected!");
      vTaskDelete(NULL); // remove loop task
    }

    camera.drawLegendGraph();
}

void loop() {
    const long start = millis();

    camera.readImage();

    const long processingTime = millis() - start;

    uint16_t dummyX = 0, dummyY = 0;
    if (tft.getTouch(&dummyX, &dummyY))
      interpolationType++;

    tft.setCursor(0, InfoBarHeight);
    camera.drawImage(interpolationType == InterpolationType::eNone ? 9 : 3, interpolationType);
    camera.drawLegendText();
    camera.drawCenterMeasurement();

    const long frameTime = millis() - start;

    Serial.printf("Draw: %d\n", frameTime - processingTime);

    infoBar.update(start, processingTime, frameTime);
    if (frameTime < MaxFrameTimeInMillis)
    delay(MaxFrameTimeInMillis - frameTime);
}
