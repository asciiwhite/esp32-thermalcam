#include "mlxcamera.h"
#include "image.h"
#include "perfcounter.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

paramsMLX90640 mlx90640;

#include <Wire.h>
#include <SPI.h>

MLXCamera::MLXCamera()
{
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

  return true;
}

bool MLXCamera::isConnected() const
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  return Wire.endTransmission() == 0; //Sensor did not ACK
}

float MLXCamera::getRefreshRateInHz() const
{
   const int rate = MLX90640_GetRefreshRate(MLX90640_address);
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
   const int res = MLX90640_GetCurResolution(MLX90640_address);
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

void MLXCamera::readImage(Image &image)
{
  // TODO: check image resolution
 
  for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    {
      LOG_PERF("Read");
      
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
    }
    
    {
      LOG_PERF("Calculate");
        
      const float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);    
      const float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature  
      
      MLX90640_CalculateTo(mlx90640Frame, &mlx90640, SensorEmissivity, tr, image.data());
    }
  }
}
