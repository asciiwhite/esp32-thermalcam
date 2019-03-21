#include <Arduino.h>

float get_point(const float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);
void get_adjacents_2d(const float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
float bilinearInterpolate(float p[], float x, float y);

float get_point(const float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
  if (x >= cols)    x = cols - 1;
  if (y >= rows)    y = rows - 1;
  return p[y * cols + x];
}

void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f) {
  p[y * cols + x] = f;
}

// src is a grid src_rows * src_cols
// dest is a pre-allocated grid, dest_rows*dest_cols
void interpolate_image(const float *src, uint8_t src_rows, uint8_t src_cols, 
                       float *dest, uint8_t dest_rows, uint8_t dest_cols) {
  const float mu_x = (src_cols - 1.0) / (dest_cols - 1.0);
  const float mu_y = (src_rows - 1.0) / (dest_rows - 1.0);

  float adj_2d[4]; // matrix for storing adjacents
  
  for (uint8_t y_idx=0; y_idx < dest_rows; y_idx++) {
    for (uint8_t x_idx=0; x_idx < dest_cols; x_idx++) {
       float x = x_idx * mu_x;
       float y = y_idx * mu_y;
       get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);

       float frac_x = x - (int)x; // we only need the ~delta~ between the points
       float frac_y = y - (int)y; // we only need the ~delta~ between the points
       float out = bilinearInterpolate(adj_2d, frac_x, frac_y);
       set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
    }
  }
}

// p is a 4-point 2x2 array of the rows & columns left/right/above/below
float bilinearInterpolate(float p[], float x, float y) {
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
