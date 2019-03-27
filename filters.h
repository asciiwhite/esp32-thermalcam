#ifndef H_FILTERS
#define H_FILTERS

#include <Arduino.h>

class KalmanFilter
{
public:

  // input variance determined using excel and reading samples of raw sensor data
  KalmanFilter(float _varianceProcess, float _inputVariance)
  : varianceProcess(_varianceProcess)
  , inputVariance(_inputVariance)
  {
  }

  float process(float measurment)
  {
    Pc = P + varianceProcess;
    G = Pc / (Pc + inputVariance);    // kalman gain
    P = (1 - G) * Pc;
    Xp = Xe;
    Zp = Xp;
    Xe = G * (measurment - Zp) + Xp; // the kalman estimate of the sensor voltage
    return Xe;
  }

private:
  float inputVariance = 0.f;  
  float varianceProcess = 1e-8;
  float Pc = 0.0;
  float G = 0.0;
  float P = 1.0;
  float Xp = 0.0;
  float Zp = 0.0;
  float Xe = 0.0;  
};

// exponential filtering https://en.wikipedia.org/wiki/Exponential_smoothing
inline float filterExponentional(float measurement, float lastFilteredValue, float smoothingFactor) {
  return measurement * smoothingFactor + lastFilteredValue * (1.f - smoothingFactor);
};

#endif
