#include <Arduino.h>

class KalmanFilter
{
public:
  KalmanFilter(float process)
  : varProcess(process)
  {
  }

  float process(float input)
  {
    Pc = P + varProcess;
    G = Pc / (Pc + varVolt);    // kalman gain
    P = (1 - G) * Pc;
    Xp = Xe;
    Zp = Xp;
    Xe = G * (input - Zp) + Xp; // the kalman estimate of the sensor voltage
    return Xe;
  }

private:
  float varVolt = 1.12184278324081E-05;  // variance determined using excel and reading samples of raw sensor data
  float varProcess = 1e-8;
  float Pc = 0.0;
  float G = 0.0;
  float P = 1.0;
  float Xp = 0.0;
  float Zp = 0.0;
  float Xe = 0.0;  
};

float getBatteryVoltage() {
  const float noisyVoltage = 1.1f // ADC Reference Voltage of 1100mV
        * 2.f            // voltage is halved
        * 3.3f           // reference voltage of the ESP32
        * analogRead(35) // LiPo battery
        / 4095.0f;       // 12-bit precision 

  static KalmanFilter voltageFilter(1e-8);
  return voltageFilter.process(noisyVoltage);
}
