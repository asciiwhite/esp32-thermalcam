#ifndef H_MLXCAMERA
#define H_MLXCAMERA

#include <Arduino.h>

class TFT_eSPI;

class MLXCamera
{
public:
    MLXCamera(TFT_eSPI& tft);

    bool init();

    void readImage();

    void drawImage() const;
    void drawLegend() const;
    void drawCenterMeasurement() const;

private:
    void readPixels();
    void setTempScale();
    void setAbcd();
    uint16_t getColor(float val) const;

    TFT_eSPI& tft;

    // array for the 32 x 24 measured pixels
    float pixels[768];

    float minTemp = 20.0;
    float maxTemp = 40.0;

    // cutoff points for temp to RGB conversion
    float a = 0.0;
    float b = 0.0;
    float c = 0.0;
    float d = 0.0;
    
    //Default 7-bit unshifted address of the MLX90640
    static const byte MLX90640_address = 0x33;
};

#endif
