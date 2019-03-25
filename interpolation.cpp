#include <Arduino.h>

void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f) {
  p[y * cols + x] = f;
}

namespace bilinear
{
  float get_point(const float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
    if (x >= cols)    x = cols - 1;
    if (y >= rows)    y = rows - 1;
    return p[y * cols + x];
  }

  // p is a 4-point 2x2 array of the rows & columns left/right/above/below
  float interpolate(const float p[], float x, float y) {
      float xx = p[1] * x + p[0] * (1.f - x);
      float xy = p[3] * x + p[2] * (1.f - x);
      return xy * y + xx * (1.f - y);
  }
  
  // src is rows*cols and dest is a 4-point array passed in already allocated!
  void get_adjacents_2d(const float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
    dest[0] = get_point(src, rows, cols, x+0, y+0);
    dest[1] = get_point(src, rows, cols, x+1, y+0);
    dest[2] = get_point(src, rows, cols, x+0, y+1);
    dest[3] = get_point(src, rows, cols, x+1, y+1);
  }
}

// src is a grid src_rows * src_cols
// dest is a pre-allocated grid, dest_rows*dest_cols
void interpolate_image_bilinear(const float *src, uint8_t src_rows, uint8_t src_cols, 
                                float *dest, uint8_t dest_rows, uint8_t dest_cols, uint8_t upScaleFactor) {

  const float mu = 1.f / upScaleFactor;
  float adj_2d[4]; // matrix for storing adjacents
  
  for (uint8_t y_idx=0; y_idx < dest_rows; y_idx++) {
    for (uint8_t x_idx=0; x_idx < dest_cols; x_idx++) {
       float x = x_idx * mu;
       float y = y_idx * mu;
       bilinear::get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);

       float frac_x = x - (int)x; // we only need the ~delta~ between the points
       float frac_y = y - (int)y; // we only need the ~delta~ between the points
       float out = bilinear::interpolate(adj_2d, frac_x, frac_y);
       set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
    }
  }
}


namespace bicubic
{
  float get_point(const float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
    if (x < 0)        x = 0;
    if (y < 0)        y = 0;
    if (x >= cols)    x = cols - 1;
    if (y >= rows)    y = rows - 1;
    return p[y * cols + x];
  }
  
  // p is a list of 4 points, 2 to the left, 2 to the right
  float cubicInterpolate(const float p[], float x) {
      return p[1] + (0.5 * x * (p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0]))));
  }
  
  // p is a 16-point 4x4 array of the 2 rows & columns left/right/above/below
  float interpolate(const float p[], float x, float y) {
      float arr[4] = {0,0,0,0};
      arr[0] = cubicInterpolate(p+0, x);
      arr[1] = cubicInterpolate(p+4, x);
      arr[2] = cubicInterpolate(p+8, x);
      arr[3] = cubicInterpolate(p+12, x);
      return cubicInterpolate(arr, y);
  }

  // src is rows*cols and dest is a 16-point array passed in already allocated!
  void get_adjacents_2d(const float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
      float arr[4];
      for (int8_t delta_y = -1; delta_y < 3; delta_y++) { // -1, 0, 1, 2
          float *row = dest + 4 * (delta_y+1); // index into each chunk of 4
          for (int8_t delta_x = -1; delta_x < 3; delta_x++) { // -1, 0, 1, 2
              row[delta_x+1] = get_point(src, rows, cols, x+delta_x, y+delta_y);
          }
      }
  }
}

// src is a grid src_rows * src_cols
// dest is a pre-allocated grid, dest_rows*dest_cols
void interpolate_image_bicubic(const float *src, uint8_t src_rows, uint8_t src_cols, 
                               float *dest, uint8_t dest_rows, uint8_t dest_cols, uint8_t upScaleFactor) {
  const float mu = 1.f / upScaleFactor;
  float adj_2d[16]; // matrix for storing adjacents
  
  for (uint8_t y_idx=0; y_idx < dest_rows; y_idx++) {
    for (uint8_t x_idx=0; x_idx < dest_cols; x_idx++) {
       float x = x_idx * mu;
       float y = y_idx * mu;
       bicubic::get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);

       float frac_x = x - (int)x; // we only need the ~delta~ between the points
       float frac_y = y - (int)y; // we only need the ~delta~ between the points
       float out = bicubic::interpolate(adj_2d, frac_x, frac_y);
       set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
    }
  }
}
