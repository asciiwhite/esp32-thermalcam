#include "mlxcamera.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

paramsMLX90640 mlx90640;
#define TA_SHIFT 8 //Default shift for MLX90640 in open air

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

MLXCamera::MLXCamera(TFT_eSPI& _tft)
 : tft(_tft)
{
  for (int i = 0; i < 768; i++)
    pixels[i] = random(0, 41);
}

bool MLXCamera::init()
{
  // Connect thermal sensor.
  Wire.begin();
  Wire.setClock(400000); // Increase I2C clock speed to 400kHz
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
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

  // Set refresh rate
  MLX90640_SetRefreshRate(MLX90640_address, 0x05); // Set rate to 8Hz effective - Works at 800kHz
  
  // Once EEPROM has been read at 400kHz we can increase
  Wire.setClock(800000);

  SPI.begin();
  SPI.setFrequency(80000000L);  

  return true;
}

void MLXCamera::readImage()
{
//  readPixels();
  setTempScale();
}

// Read pixel data from MLX90640.
void MLXCamera::readPixels()
{
  float emissivity = 0.95;
  
  for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }
    
//    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
    
    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature  
    
    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, pixels);
  }
}

// Get color for temp value.
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
  minTemp = 255;
  maxTemp = 0;

  for (int i = 0; i < 768; i++) {
    minTemp = min(minTemp, pixels[i]);
    maxTemp = max(maxTemp, pixels[i]);
  }

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

void MLXCamera::drawImage() const
{
  for (int y=0; y<24; y++) {
    for (int x=0; x<32; x++) {
     tft.fillRect(tft.cursor_x + 8 + x*7, tft.cursor_y + 8 + y*7, 7, 7, getColor(pixels[(31-x) + (y*32)]));
    }
  }
}

void MLXCamera::drawLegend() const
{
  const int legendSize = 20;
  float inc = (maxTemp - minTemp) / tft.width();
  int j = 0;
  for (float ii = minTemp; ii < maxTemp; ii += inc) {
    tft.drawFastVLine(tft.cursor_x + j++, tft.height() - legendSize, legendSize, getColor(ii));
  }

  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, tft.height() - legendSize - 10);
  tft.print(String(minTemp).substring(0, 4));
  tft.setCursor(tft.width() - 25, tft.height() - legendSize - 10);
  tft.print(String(maxTemp).substring(0, 4));
}

// Draw a circle + measured value.
void MLXCamera::drawCenterMeasurement() const
{
  // Mark center measurement
  tft.drawCircle(120, 8+84, 3, TFT_WHITE);

  // Measure and print center temperature
  const float centerTemp = (pixels[383 - 16] + pixels[383 - 15] + pixels[384 + 15] + pixels[384 + 16]) / 4;
  tft.setCursor(86, 214);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.print(String(centerTemp).substring(0, 4) + " °C");
}
