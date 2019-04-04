#include "renderer.h"
#include "interpolation.h"
#include "filters.h"
#include "perfcounter.h"

#include <TFT_eSPI.h>

namespace {
  float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

SensorRenderer::SensorRenderer(TFT_eSPI& _tft)
 : tft(_tft)
 , filteredPixels(SensorWidth, SensorHeight)
 , upscaledPixels(UpScaledWidth, UpScaledHeight)
{
}

void SensorRenderer::setFixedTemperatureRange()
{
  fixedTemperatureRange = true;
  minTemp = DefaultMinTemp;
  maxTemp = DefaultMaxTemp;
}

void SensorRenderer::setDynamicTemperatureRange()
{
  fixedTemperatureRange = false;
}

uint16_t SensorRenderer::getFalseColor(float value) const
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

uint16_t SensorRenderer::getColor(float val) const
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

void SensorRenderer::setTempScale()
{
  if (fixedTemperatureRange)
    return;

  const auto minmax = std::minmax_element(filteredPixels.begin(), filteredPixels.end());
  if (!std::isnan(*minmax.first) && !std::isnan(*minmax.second))  
  {  
    minTemp = *minmax.first;
    maxTemp = *minmax.second;
  }

  setAbcd();
}

// Function to get the cutoff points in the temp vs RGB graph.
void SensorRenderer::setAbcd()
{
  a = minTemp + (maxTemp - minTemp) * 0.2121;
  b = minTemp + (maxTemp - minTemp) * 0.3182;
  c = minTemp + (maxTemp - minTemp) * 0.4242;
  d = minTemp + (maxTemp - minTemp) * 0.8182;
}

void SensorRenderer::drawImage(const Image& image, int scale) const
{
  LOG_PERF("Draw");
    
  for (int y=0; y<image.height; y++) {
    for (int x=0; x<image.width; x++) {
      tft.fillRect(tft.cursor_x + x*scale, tft.cursor_y + 10 + y*scale, scale, scale, getFalseColor(image[(image.width-1-x) + (y*image.width)]));
    }
  }
}

void SensorRenderer::denoiseRawPixels(const Image& measuredImage)
{
  LOG_PERF("Denoising");

  for (int i = 0; i < SensorWidth * SensorHeight; i++)
      filteredPixels[i] = filterExponentional(measuredImage[i], filteredPixels[i], DenoisingSmoothingFactor);
}

void SensorRenderer::interpolateImage(InterpolationType interpolationType)
{
  LOG_PERF("Interpolation");

  if (interpolationType == InterpolationType::eLinear)
    interpolate_image_bilinear(filteredPixels.data(), filteredPixels.height, filteredPixels.width, upscaledPixels.data(), upscaledPixels.height, upscaledPixels.width, UpScaleFactor);
  else
    interpolate_image_bicubic(filteredPixels.data(), filteredPixels.height, filteredPixels.width, upscaledPixels.data(), upscaledPixels.height, upscaledPixels.width, UpScaleFactor);
}

void SensorRenderer::drawImage(const Image& image, InterpolationType interpolationType)
{
  denoiseRawPixels(image);
  setTempScale();

  if (interpolationType == InterpolationType::eNone)
  {
    drawImage(filteredPixels, 9);
  }
  else
  {
    interpolateImage(interpolationType);    
    drawImage(upscaledPixels, 3);
  }
}

void SensorRenderer::drawLegendGraph() const
{
  const int legendSize = 15;    
  const float inc = (maxTemp - minTemp) / (tft.height() - 25 - 25);
  int j = 0;
  for (float ii = maxTemp; ii >= minTemp; ii -= inc)
    tft.drawFastHLine(tft.width() - legendSize - 6, tft.cursor_y + 34 + j++, legendSize, getFalseColor(ii));
}
 
void SensorRenderer::drawLegendText() const
{  
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(tft.width() - 25, tft.height() - 10);
  tft.print(String(minTemp).substring(0, 4));
  tft.setCursor(tft.width() - 25, 20);
  tft.print(String(maxTemp).substring(0, 4));
}

void SensorRenderer::drawCenterMeasurement() const
{
  const int32_t centerX = SensorWidth  * 9 / 2;
  const int32_t centerY = SensorHeight * 9 / 2 + 20 - 1;
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
