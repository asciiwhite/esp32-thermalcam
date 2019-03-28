#ifndef H_RENDERER
#define H_RENDERER

#include "image.h"

#include <Arduino.h>
#include <array>

class TFT_eSPI;

enum class InterpolationType {
  eNone,
  eLinear,
  eCubic
};

inline InterpolationType& operator++(InterpolationType& type, int)
{
    if (type == InterpolationType::eCubic)
      type = InterpolationType::eNone;
    else
      type = static_cast<InterpolationType>(static_cast<int>(type) + 1);
    return type;
};

class SensorRenderer
{
  public:
    SensorRenderer(TFT_eSPI& tft);

    void drawImage(const Image& image, InterpolationType);
    void drawLegendGraph() const;
    void drawLegendText() const;
    void drawCenterMeasurement() const;

    void setFixedTemperatureRange();
    void setDynamicTemperatureRange();

  private:
    void setTempScale();
    void setAbcd();
    uint16_t getColor(float val) const;
    uint16_t getFalseColor(float val) const;
    void drawImage(const Image& image, int scale) const;
    void denoiseRawPixels(const Image& image);
    void interpolateImage(InterpolationType interpolationType);
    
    TFT_eSPI& tft;

    // TODO : use Image instead array
    static constexpr int SensorWidth  = 32;
    static constexpr int SensorHeight = 24;
    Image filteredPixels;
    //std::array<float, SensorWidth * SensorHeight> filteredPixels;

    static constexpr int UpScaleFactor  = 3;
    static constexpr int UpScaledWidth  = (SensorWidth  - 1) * UpScaleFactor + 1;
    static constexpr int UpScaledHeight = (SensorHeight - 1) * UpScaleFactor + 1;
    Image upscaledPixels;
    //std::array<float, UpScaledWidth * UpScaledHeight> upscaledPixels;

    static constexpr float DefaultMinTemp = 20.f;
    static constexpr float DefaultMaxTemp = 45.f;
    float minTemp = DefaultMinTemp;
    float maxTemp = DefaultMaxTemp;
    bool fixedTemperatureRange = true;

    static constexpr float DenoisingSmoothingFactor = 0.4f;

    // cutoff points for temp to RGB conversion
    float a = 0.0;
    float b = 0.0;
    float c = 0.0;
    float d = 0.0;
};

#endif
