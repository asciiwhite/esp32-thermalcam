#ifndef H_MLXCAMERA
#define H_MLXCAMERA

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

class MLXCamera
{
public:
    MLXCamera(TFT_eSPI& tft);

    bool init();
    bool isConnected() const;

    void readImage();
    
    void drawImage(InterpolationType);
    void drawLegendGraph() const;
    void drawLegendText() const;
    void drawCenterMeasurement() const;

    void setFixedTemperatureRange();
    void setDynamicTemperatureRange();

private:
    void readPixels();
    void setTempScale();
    void setAbcd();
    uint16_t getColor(float val) const;
    uint16_t getFalseColor(float val) const;
    void drawImage(const float *pixelData, int width, int height, int scale) const;
    void denoiseRawPixels(const float smoothingFactor);
    void interpolateImage(InterpolationType interpolationType);

    float getRefreshRateInHz() const;
    int getResolutionInBit() const;
    bool isInterleaved() const;
    bool isChessMode() const;

    TFT_eSPI& tft;

    static constexpr int sensorWidth  = 32;
    static constexpr int sensorHeight = 24;    
    std::array<float, sensorWidth * sensorHeight> measuredPixels;
    std::array<float, sensorWidth * sensorHeight> filteredPixels;

    static constexpr int upscaleFactor  = 3;
    static constexpr int upScaledWidth  = (sensorWidth  - 1) * upscaleFactor + 1;
    static constexpr int upScaledHeight = (sensorHeight - 1) * upscaleFactor + 1;
    std::array<float, upScaledWidth * upScaledHeight> upscaledPixels;

    static constexpr float defaultMinTemp = 20.f;
    static constexpr float defaultMaxTemp = 45.f;
    float minTemp = defaultMinTemp;
    float maxTemp = defaultMaxTemp;
    bool fixedTemperatureRange = true;

    static constexpr float denoisingSmoothingFactor = 0.4f;
    static constexpr float sensorEmissivity = 0.95f;

    // cutoff points for temp to RGB conversion
    float a = 0.0;
    float b = 0.0;
    float c = 0.0;
    float d = 0.0;
    
    //Default 7-bit unshifted address of the MLX90640
    static const byte MLX90640_address = 0x33;
};

#endif
