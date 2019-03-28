#ifndef H_IMAGE
#define H_IMAGE

#include <vector>

struct Image : private std::vector<float>
{
public:
  Image(int _width, int _height)
  : width(_width)
  , height(_height)
  {
    resize(width * height, 0.f);
  }
  
  const int width;
  const int height;

  using std::vector<float>::begin;
  using std::vector<float>::end;
  using std::vector<float>::data;
  using std::vector<float>::operator[];
};

#endif
