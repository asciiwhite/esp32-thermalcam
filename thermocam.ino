#include "infobar.h"
#include "mlxcamera.h"
#include "renderer.h"
#include "image.h"

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

MLXCamera camera;
SensorRenderer renderer(tft);
InfoBar infoBar = InfoBar(tft);
const uint32_t InfoBarHeight = 10;
const uint32_t MaxFrameTimeInMillis = 33;

InterpolationType interpolationType = InterpolationType::eLinear;
bool fixedTemperatureRange = true;

#ifdef min
#undef min
#endif

#include <queue>
#include <mutex>
#include <condition_variable>

struct SafeImage {
  std::mutex m;
  std::condition_variable cv;
  std::queue<int> queue;
};

SafeImage readSensorQueue;
SafeImage presentationQueue;

Image images[] = { Image(32, 24), Image(32, 24) };

long frameId = 0;
long startTime = 0;
long processingTime = 0;

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

    readSensorQueue.queue.push(0);
    readSensorQueue.queue.push(1);

    xTaskCreatePinnedToCore(
                    presentationTask,   /* Function to implement the task */
                    "coreTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    0);  /* Core where the task should run */

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
    Serial.printf("Frame %d: Wait for sensor read\n", frameId);

    int readyImageID = 0;
  {
    std::unique_lock<std::mutex> lk(readSensorQueue.m);
    readSensorQueue.cv.wait(lk, []{return !readSensorQueue.queue.empty();});
    readyImageID = readSensorQueue.queue.front();
    readSensorQueue.queue.pop();
  }
  
  Serial.printf("Frame %d: Do sensor reading bufferId: %d\n", frameId, readyImageID);
  
  startTime = millis();
  camera.readImage(images[readyImageID]);
  processingTime = millis() - startTime; 

  std::lock_guard<std::mutex> lk(presentationQueue.m);
  presentationQueue.queue.push(readyImageID);
  presentationQueue.cv.notify_one();
}

void presentationTask(void*) {
  while (1) {
    Serial.printf("Frame %d: Wait for presentation\n", frameId);

    int presentImageID = 0;
    {
      std::unique_lock<std::mutex> lk(presentationQueue.m);
      presentationQueue.cv.wait(lk, []{return !presentationQueue.queue.empty();});
      presentImageID = presentationQueue.queue.front();
      presentationQueue.queue.pop();
    }
  
    Serial.printf("Frame %d: Do presentation bufferId: %d\n", frameId, presentImageID);
      
  //    const long start = millis();
  
    handleTouch();
    
    renderer.drawCenterMeasurement();
    renderer.drawLegendText();
    tft.setCursor(0, InfoBarHeight);
    renderer.drawImage(images[presentImageID], interpolationType);

    const long frameTime = millis() - startTime;

    infoBar.update(startTime, processingTime, frameTime);
    if (frameTime < MaxFrameTimeInMillis)
      delay(MaxFrameTimeInMillis - frameTime);

    std::lock_guard<std::mutex> lk(readSensorQueue.m);
    readSensorQueue.queue.push(presentImageID);
    readSensorQueue.cv.notify_one();
    
    frameId++;
  }
}
