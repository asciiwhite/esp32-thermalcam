#include "mlxcamera.h"
#include "interpolation.h"
#include "filters.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

paramsMLX90640 mlx90640;
#define TA_SHIFT 8 //Default shift for MLX90640 in open air

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

//#define DEBUG_INTERPOLATION

namespace {
  float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

MLXCamera::MLXCamera(TFT_eSPI& _tft)
 : tft(_tft)
{
#ifdef DEBUG_INTERPOLATION
   for (int i = 0; i < sensorWidth * sensorHeight; i++)
    measuredPixels[i] = ((i + i / sensorWidth) % 2) == 0 ? 20.f : 30.f;
#endif
}

bool MLXCamera::init()
{
  // Connect thermal sensor.
  Wire.begin();
  Wire.setClock(400000); // Increase I2C clock speed to 400kHz

  if (!isConnected())
  {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring.");
    return false;
  }
    
  Serial.println("MLX90640 online!");
    
  // Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
  {
    Serial.println("Failed to load system parameters");
    return false;
  }
    
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
  {
    Serial.println("Parameter extraction failed");
    return false;
  }

  MLX90640_SetChessMode(MLX90640_address);
  status = MLX90640_SetRefreshRate(MLX90640_address, 0x05); // Set rate to 8Hz effective - Works at 800kHz
  if (status != 0)
  {
    Serial.println("SetRefreshRate failed");
    return false;
  }
  
  Serial.printf("RefreshRate: %.1f Hz\n", getRefreshRateInHz());
  Serial.printf("Resolution: %d-bit\n", getResolutionInBit());
  if (isInterleaved())
    Serial.println("Mode: Interleaved");
  else
    Serial.println("Mode: Chess");

  
  // Once EEPROM has been read at 400kHz we can increase
  Wire.setClock(800000);

  SPI.begin();
  SPI.setFrequency(80000000L);  

  return true;
}

bool MLXCamera::isConnected() const
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  return Wire.endTransmission() == 0; //Sensor did not ACK
}

float MLXCamera::getRefreshRateInHz() const
{
   int rate = MLX90640_GetRefreshRate(MLX90640_address);
   switch(rate) {
    case 0: return 0.5f;
    case 1: return 1.f;
    case 2: return 2.f;
    case 3: return 4.f;
    case 4: return 8.f;
    case 5: return 16.f;
    case 6: return 32.f;
    case 7: return 64.f;
    default : return 0.f;
   }
}

int MLXCamera::getResolutionInBit() const
{
   int res = MLX90640_GetCurResolution(MLX90640_address);
   switch(res) {
    case 0: return 16;
    case 1: return 17;
    case 2: return 18;
    case 3: return 19;
    default : return 0;
   }
}

bool MLXCamera::isInterleaved() const
{
   return MLX90640_GetCurMode(MLX90640_address) == 0;
}

bool MLXCamera::isChessMode() const
{
   return MLX90640_GetCurMode(MLX90640_address) == 1;
}

void MLXCamera::setFixedTemperatureRange()
{
  fixedTemperatureRange = true;
  minTemp = defaultMinTemp;
  maxTemp = defaultMaxTemp;
}

void MLXCamera::setDynamicTemperatureRange()
{
  fixedTemperatureRange = false;
}

void MLXCamera::readImage()
{
#ifndef DEBUG_INTERPOLATION
  readPixels();
  setTempScale();
#endif
}

void MLXCamera::readPixels()
{
  for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      if (status == -8)
      {
        // Could not aquire frame data in certain time, I2C frequency may be too low
        Serial.println("GetFrame Error: could not aquire frame data in time");
      }
      else
      {
        Serial.printf("GetFrame Error: %d\n", status);
      }
    }

    const long start = millis(); 
    
    const float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);    
    const float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature  
    
    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, sensorEmissivity, tr, measuredPixels.data());

    Serial.printf(" Calculate: %d\n", millis() - start);
  }
}

uint16_t MLXCamera::getFalseColor(float value) const
{
    // Heatmap code borrowed from: http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients
    static float color[][3] = { {0,0,0}, {0,0,255}, {0,255,0}, {255,255,0}, {255,0,0}, {255,0,255} };
//    static const float color[][3] = { {0,0,20}, {0,0,100}, {80,0,160}, {220,40,180}, {255,200,20}, {255,235,20}, {255,255,255} };

    static const int NUM_COLORS = sizeof(color) / sizeof(color[0]);
    value = (value - minTemp) / (maxTemp-minTemp);
    
    if(value <= 0.f)
    {
      return tft.color565(color[0][0], color[0][1], color[0][2]);
    }
      
    if(value >= 1.f)
    {
      return tft.color565(color[NUM_COLORS-1][0], color[NUM_COLORS-1][1], color[NUM_COLORS-1][2]);
    }

    value *= NUM_COLORS-1;
    const int idx1 = floor(value);
    const int idx2 = idx1+1;
    const float fractBetween = value - float(idx1);
  
    const byte ir = ((color[idx2][0] - color[idx1][0]) * fractBetween) + color[idx1][0];
    const byte ig = ((color[idx2][1] - color[idx1][1]) * fractBetween) + color[idx1][1];
    const byte ib = ((color[idx2][2] - color[idx1][2]) * fractBetween) + color[idx1][2];

    return tft.color565(ir, ig, ib);
}

uint16_t MLXCamera::getColor(float val) const
{
  /*
    pass in value and figure out R G B
    several published ways to do this I basically graphed R G B and developed simple linear equations
    again a 5-6-5 color display will not need accurate temp to R G B color calculation
    equations based on
    http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html
    
  */

  byte red   = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);
  byte green = 0;
  byte blue  = 0;

  if ((val > minTemp) & (val < a)) {
    green = constrain(255.0 / (a - minTemp) * val - (255.0 * minTemp) / (a - minTemp), 0, 255);
  }
  else if ((val >= a) & (val <= c)) {
    green = 255;
  }
  else if (val > c) {
    green = constrain(255.0 / (c - d) * val - (d * 255.0) / (c - d), 0, 255);
  }
  else if ((val > d) | (val < a)) {
    green = 0;
  }

  if (val <= b) {
    blue = constrain(255.0 / (a - b) * val - (255.0 * b) / (a - b), 0, 255);
  }
  else if ((val > b) & (val <= d)) {
    blue = 0;
  }
  else if (val > d) {
    blue = constrain(240.0 / (maxTemp - d) * val - (d * 240.0) / (maxTemp - d), 0, 240);
  }

  // use the displays color mapping function to get 5-6-5 color palet (R=5 bits, G=6 bits, B-5 bits)
  return tft.color565(red, green, blue);
}

void MLXCamera::setTempScale()
{
  if (fixedTemperatureRange)
    return;

  const auto minmax = std::minmax_element(filteredPixels.begin(), filteredPixels.end());
  minTemp = *minmax.first;
  maxTemp = *minmax.second;
  
  setAbcd();
}

// Function to get the cutoff points in the temp vs RGB graph.
void MLXCamera::setAbcd()
{
  a = minTemp + (maxTemp - minTemp) * 0.2121;
  b = minTemp + (maxTemp - minTemp) * 0.3182;
  c = minTemp + (maxTemp - minTemp) * 0.4242;
  d = minTemp + (maxTemp - minTemp) * 0.8182;
}

void MLXCamera::drawImage(const float *pixelData, int width, int height, int scale) const
{
  const long start = millis();
    
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      tft.fillRect(tft.cursor_x + x*scale, tft.cursor_y + 10 + y*scale, scale, scale, getFalseColor(pixelData[(width-1-x) + (y*width)]));
    }
  }

  Serial.printf("Draw: %d\n", millis() - start);
}

void MLXCamera::denoiseRawPixels(const float smoothingFactor)
{
  const long start = millis();

  for (int i = 0; i < sensorWidth * sensorHeight; i++)
      filteredPixels[i] = filterExponentional(measuredPixels[i], filteredPixels[i], smoothingFactor);

  Serial.printf("Denoising: %d ", millis() - start);
}

void MLXCamera::interpolateImage(InterpolationType interpolationType)
{
  const long start = millis();

  if (interpolationType == InterpolationType::eLinear)
    interpolate_image_bilinear(filteredPixels.data(), sensorHeight, sensorWidth, upscaledPixels.data(), upScaledHeight, upScaledWidth, upscaleFactor);
  else
    interpolate_image_bicubic(filteredPixels.data(), sensorHeight, sensorWidth, upscaledPixels.data(), upScaledHeight, upScaledWidth, upscaleFactor);

  Serial.printf("Interpolation: %d ", millis() - start);
}

void MLXCamera::drawImage(InterpolationType interpolationType)
{
  denoiseRawPixels(denoisingSmoothingFactor);

  if (interpolationType == InterpolationType::eNone)
  {
    drawImage(filteredPixels.data(), sensorWidth, sensorHeight, 9);
  }
  else
  {
    interpolateImage(interpolationType);    
    drawImage(upscaledPixels.data(), upScaledWidth, upScaledHeight, 3);
  }
}

void MLXCamera::drawLegendGraph() const
{
  const int legendSize = 15;    
  const float inc = (maxTemp - minTemp) / (tft.height() - 25 - 25);
  int j = 0;
  for (float ii = maxTemp; ii >= minTemp; ii -= inc)
    tft.drawFastHLine(tft.width() - legendSize - 6, tft.cursor_y + 34 + j++, legendSize, getFalseColor(ii));
}
 
void MLXCamera::drawLegendText() const
{
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(tft.width() - 25, tft.height() - 10);
  tft.print(String(minTemp).substring(0, 4));
  tft.setCursor(tft.width() - 25, 20);
  tft.print(String(maxTemp).substring(0, 4));
}

void MLXCamera::drawCenterMeasurement() const
{
  const int32_t centerX = sensorWidth  * 9 / 2;
  const int32_t centerY = sensorHeight * 9 / 2 + 20 - 1;
  const int32_t halfCrossSize = 3; 
  tft.drawFastHLine(centerX - halfCrossSize, centerY, 2 * halfCrossSize + 1, TFT_WHITE);
  tft.drawFastVLine(centerX, centerY - halfCrossSize, 2 * halfCrossSize + 1, TFT_WHITE);
  
  const float avgCenterTemperature = (filteredPixels[383 - 16] + filteredPixels[383 - 15] + filteredPixels[384 + 15] + filteredPixels[384 + 16]) * 0.25f;
  const int32_t legendPixelLength = tft.height() - 25 - 25;
  const int32_t centerOffset = mapf(avgCenterTemperature, minTemp, maxTemp, 0, legendPixelLength);
  
  const int32_t positionX = 296; //tft.width() - legendSize - 6
  const int32_t positionY = tft.height() - 15 - centerOffset;

  const int32_t height = 6;
  const int32_t width  = 6;

  static int32_t x0;
  static int32_t y0;
  static int32_t x1;
  static int32_t y1;
  static int32_t x2;
  static int32_t y2;

  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_BLACK);
  
  x0 = positionX;
  y0 = positionY;
  x1 = positionX - width;
  y1 = positionY - (height / 2);
  x2 = x1;
  y2 = positionY + (height / 2);
  
  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_WHITE);
}
