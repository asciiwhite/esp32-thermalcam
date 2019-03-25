#ifndef H_INTERPOLATION
#define H_INTERPOLATION

void interpolate_image_bilinear(const float *src, uint8_t src_rows, uint8_t src_cols, 
                                float *dest, uint8_t dest_rows, uint8_t dest_cols, uint8_t upScaleFactor);

void interpolate_image_bicubic(const float *src, uint8_t src_rows, uint8_t src_cols, 
                               float *dest, uint8_t dest_rows, uint8_t dest_cols, uint8_t upScaleFactor);

#endif
