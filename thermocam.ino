#include "infobar.h"
#include "mlxcamera.h"
#include "renderer.h"
#include "image.h"

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

MLXCamera camera;
SensorRenderer renderer(tft);
Image sensorImage(32, 24);

InfoBar infoBar = InfoBar(tft);
const uint32_t InfoBarHeight = 10;
const uint32_t MaxFrameTimeInMillis = 33;

InterpolationType interpolationType = InterpolationType::eLinear;
bool fixedTemperatureRange = true;

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

    renderer.drawLegendGraph();
}

void handleTouch()
{
  uint16_t dummyX = 0, dummyY = 0;
  if (tft.getTouch(&dummyX, &dummyY))
  {
    if (dummyX > 80)
      interpolationType++;
    else
    {
      fixedTemperatureRange = !fixedTemperatureRange;
      if (fixedTemperatureRange)
        renderer.setFixedTemperatureRange();
      else
        renderer.setDynamicTemperatureRange();
    }
  }
}

void loop() {
    const long start = millis();

    camera.readImage(sensorImage);

    const long processingTime = millis() - start;

    handleTouch();
    
    renderer.drawCenterMeasurement();
    renderer.drawLegendText();
    tft.setCursor(0, InfoBarHeight);
    renderer.drawImage(sensorImage, interpolationType);

    const long frameTime = millis() - start;

    infoBar.update(start, processingTime, frameTime);
    if (frameTime < MaxFrameTimeInMillis)
      delay(MaxFrameTimeInMillis - frameTime);
}
