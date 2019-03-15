#include <Arduino.h>

float getBatteryVoltage() {
  return  1.1f           // ADC Reference Voltage of 1100mV
        * 2.f            // voltage is halved
        * 3.3f           // reference voltage of the ESP32
        * analogRead(35) // LiPo battery
        / 4095.0f;       // 12-bit precision 
}
