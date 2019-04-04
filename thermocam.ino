#include "infobar.h"
#include "mlxcamera.h"
#include "renderer.h"
#include "image.h"
#include "perfcounter.h"

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

template<typename T> 
class ThreadSafeQueue
{
public:
  T front_and_pop() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this]{return !queue.empty();});
    T resultValue = queue.front();
    queue.pop();
    return resultValue;
  };

  void push(T value) {
    std::lock_guard<std::mutex> lock(m);
    queue.push(value);
    cv.notify_one();
  }

private:
  std::mutex m;
  std::condition_variable cv;
  std::queue<T> queue;
};

ThreadSafeQueue<int> readSensorQueue;
ThreadSafeQueue<int> presentationQueue;

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

    readSensorQueue.push(0);
    readSensorQueue.push(1);

    xTaskCreatePinnedToCore(
                    presentationTask,   /* Function to implement the task */
                    "presentationTask", /* Name of the task */
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
  const int readyImageID = readSensorQueue.front_and_pop();
  
  startTime = millis();
  camera.readImage(images[readyImageID]);
  processingTime = millis() - startTime; 

  presentationQueue.push(readyImageID);
}

void presentationTask(void*) {
  while (1) {
    const int presentImageID = presentationQueue.front_and_pop();

    tft.setCursor(0, InfoBarHeight);
    renderer.drawImage(images[presentImageID], interpolationType);

    {
      LOG_PERF("Legend&Touch");
      renderer.drawCenterMeasurement();
      renderer.drawLegendText();
      handleTouch();
    }

    const long frameTime = millis() - startTime;

    infoBar.update(startTime, processingTime, frameTime);
    if (frameTime < MaxFrameTimeInMillis)
      delay(MaxFrameTimeInMillis - frameTime);

    LOG_PRINT;
    frameId++;

    readSensorQueue.push(presentImageID);
  }
}
