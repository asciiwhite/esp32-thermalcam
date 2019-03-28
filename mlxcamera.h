#ifndef H_MLXCAMERA
#define H_MLXCAMERA

#include <Arduino.h>

struct Image;

class MLXCamera
{
public:
    MLXCamera();

    bool init();
    bool isConnected() const;

    void readImage(Image& image);

private:
    float getRefreshRateInHz() const;
    int getResolutionInBit() const;
    bool isInterleaved() const;
    bool isChessMode() const;

    static constexpr byte MLX90640_address  = 0x33;     //Default 7-bit unshifted address of the MLX90640
    static constexpr float TA_SHIFT         = 8.f;      //Default shift for MLX90640 in open air
    static constexpr float SensorEmissivity = 0.95f;
};

#endif
