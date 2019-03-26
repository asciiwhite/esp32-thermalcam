#ifndef H_MLXCAMERA
#define H_MLXCAMERA

#include <Arduino.h>

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
    
    void drawImage(int scale, InterpolationType) const;
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
    void denoiseRawPixels(const float smoothingFactor) const;

    float getRefreshRateInHz() const;
    int getResolutionInBit() const;
    bool isInterleaved() const;
    bool isChessMode() const;

    TFT_eSPI& tft;

    // array for the 32 x 24 measured pixels
    float rawPixels[768] = {0};
    mutable float filteredPixels[768]= {0};

    bool fixedTemperatureRange = true;
    float minTemp = 20.0;
    float maxTemp = 45.0;

    // cutoff points for temp to RGB conversion
    float a = 0.0;
    float b = 0.0;
    float c = 0.0;
    float d = 0.0;
    
    //Default 7-bit unshifted address of the MLX90640
    static const byte MLX90640_address = 0x33;
};

#endif
