#include "texture.h"
#include "CGL/color.h"

#include <cmath>
#include <algorithm>

namespace CGL {

Color Texture::sample(const SampleParams &sp) {
  // Parts 5 and 6: Fill this in.
  // Should return a color sampled based on the psm and lsm parameters given
  if (sp.lsm == L_ZERO) {
      if (sp.psm == P_NEAREST) {
          return sample_nearest(sp.p_uv, 0);
      } else if (sp.psm == P_LINEAR) {
          return sample_bilinear(sp.p_uv, 0);
      }
  } else if (sp.lsm == L_NEAREST) {
      int level = get_level(sp) + 0.5; //round the value to get the nearest level
      if (sp.psm == P_NEAREST) {
          return sample_nearest(sp.p_uv, 1);
      } else if (sp.psm == P_LINEAR) {
          return sample_bilinear(sp.p_uv, level);
      }
  } else if (sp.lsm == L_LINEAR) {
      float level = get_level(sp);
      int level1 = level;
      int level2 = level + 1;
      Color color1;
      Color color2;
      if (sp.psm == P_NEAREST) {
          color1 = sample_nearest(sp.p_uv, level1);
          color2 = sample_nearest(sp.p_uv, level2);
      } else if (sp.psm == P_LINEAR) {
          color1 = sample_bilinear(sp.p_uv, level1);
          color2 = sample_bilinear(sp.p_uv, level2);
      }
      return Color(color1 * (level2 - level) + color2 * (level - level1));
  }

}

float Texture::get_level(const SampleParams &sp) {
  // Optional helper function for Parts 5 and 6
  Vector2D x_diff = sp.p_dx_uv - sp.p_uv;
  Vector2D y_diff = sp.p_dy_uv - sp.p_uv;
  float L1 = sqrt(pow(x_diff[0] * width, 2) + pow(x_diff[1] * height, 2));
  float L2 = sqrt(pow(y_diff[0] * width, 2) + pow(y_diff[1] * height, 2));
  float level = log2f(max(L1, L2));
  level = level < 0 ? 0 : level;
  level = level > mipmap.size() - 1 ? mipmap.size() - 1 : level;
  return level;
}

// Returns the nearest sample given a particular level and set of uv coords
Color Texture::sample_nearest(Vector2D uv, int level) {
  // Optional helper function for Parts 5 and 6
  // Feel free to ignore or create your own
  size_t mipmap_width = mipmap[level].width;
  size_t mipmap_height = mipmap[level].height;
  int w = (uv[0] * mipmap_width + 0.5);
  int h = (uv[1] * mipmap_height + 0.5);
  return mipmap[level].get_texel(w, h);
  //+ 0.5 because needed to be rounded to the nearest value
}

// Returns the bilinear sample given a particular level and set of uv coords
Color Texture::sample_bilinear(Vector2D uv, int level) {
  // Optional helper function for Parts 5 and 6
  // Feel free to ignore or create your own
  //TODO
  size_t mipmap_width = mipmap[level].width;
  size_t mipmap_height = mipmap[level].height;
  double u = uv[0] * mipmap_width;
  double v = uv[1] * mipmap_height;
  int w = (int) u;
  int h = (int) v;
  Color u01 = mipmap[level].get_texel(w, h + 1);
  Color u11 = mipmap[level].get_texel(w + 1, h + 1);
  Color u00 = mipmap[level].get_texel(w, h);
  Color u10 = mipmap[level].get_texel(w + 1, h);

  Color u0 = (1 - u + w) * u00 + (u - w) * u10;
  Color u1 = (1 - u + w) * u01 + (u - w) * u11;
  Color final = (1 - v + h) * u0 + (v - h) * u1;
  return Color(final);
}

Color MipLevel::get_texel(int tx, int ty) {
    int x = tx < 0 ? 0 : tx;
    int y = ty < 0 ? 0 : ty;
    x = x > width - 1 ? width - 1 : x;
    y = y > height - 1 ? height - 1 : y;

    float r =  texels[(y * width + x) * 3] * 1.0 / 255.0;
    float g =  texels[(y * width + x) * 3 + 1] * 1.0 / 255.0;
    float b =  texels[(y * width + x) * 3 + 2] * 1.0 / 255.0;
    return Color(r, g, b);
}



/****************************************************************************/



inline void uint8_to_float(float dst[3], unsigned char *src) {
  uint8_t *src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
}

inline void float_to_uint8(unsigned char *dst, float src[3]) {
  uint8_t *dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[0])));
  dst_uint8[1] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[1])));
  dst_uint8[2] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[2])));
}

void Texture::generate_mips(int startLevel) {

  // make sure there's a valid texture
  if (startLevel >= mipmap.size()) {
    std::cerr << "Invalid start level";
  }

  // allocate sublevels
  int baseWidth = mipmap[startLevel].width;
  int baseHeight = mipmap[startLevel].height;
  int numSubLevels = (int)(log2f((float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  mipmap.resize(startLevel + numSubLevels + 1);

  int width = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel &level = mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width = max(1, width / 2);
    //assert (width > 0);
    height = max(1, height / 2);
    //assert (height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(3 * width * height);
  }

  // create mips
  int subLevels = numSubLevels - (startLevel + 1);
  for (int mipLevel = startLevel + 1; mipLevel < startLevel + subLevels + 1;
       mipLevel++) {

    MipLevel &prevLevel = mipmap[mipLevel - 1];
    MipLevel &currLevel = mipmap[mipLevel];

    int prevLevelPitch = prevLevel.width * 3; // 32 bit RGB
    int currLevelPitch = currLevel.width * 3; // 32 bit RGB

    unsigned char *prevLevelMem;
    unsigned char *currLevelMem;

    currLevelMem = (unsigned char *)&currLevel.texels[0];
    prevLevelMem = (unsigned char *)&prevLevel.texels[0];

    float wDecimal, wNorm, wWeight[3];
    int wSupport;
    float hDecimal, hNorm, hWeight[3];
    int hSupport;

    float result[3];
    float input[3];

    // conditional differentiates no rounding case from round down case
    if (prevLevel.width & 1) {
      wSupport = 3;
      wDecimal = 1.0f / (float)currLevel.width;
    } else {
      wSupport = 2;
      wDecimal = 0.0f;
    }

    // conditional differentiates no rounding case from round down case
    if (prevLevel.height & 1) {
      hSupport = 3;
      hDecimal = 1.0f / (float)currLevel.height;
    } else {
      hSupport = 2;
      hDecimal = 0.0f;
    }

    wNorm = 1.0f / (2.0f + wDecimal);
    hNorm = 1.0f / (2.0f + hDecimal);

    // case 1: reduction only in horizontal size (vertical size is 1)
    if (currLevel.height == prevLevel.height) {
      //assert (currLevel.height == 1);

      for (int i = 0; i < currLevel.width; i++) {
        wWeight[0] = wNorm * (1.0f - wDecimal * i);
        wWeight[1] = wNorm * 1.0f;
        wWeight[2] = wNorm * wDecimal * (i + 1);

        result[0] = result[1] = result[2] = 0.0f;

        for (int ii = 0; ii < wSupport; ii++) {
          uint8_to_float(input, prevLevelMem + 3 * (2 * i + ii));
          result[0] += wWeight[ii] * input[0];
          result[1] += wWeight[ii] * input[1];
          result[2] += wWeight[ii] * input[2];
        }

        // convert back to format of the texture
        float_to_uint8(currLevelMem + (3 * i), result);
      }

      // case 2: reduction only in vertical size (horizontal size is 1)
    } else if (currLevel.width == prevLevel.width) {
      //assert (currLevel.width == 1);

      for (int j = 0; j < currLevel.height; j++) {
        hWeight[0] = hNorm * (1.0f - hDecimal * j);
        hWeight[1] = hNorm;
        hWeight[2] = hNorm * hDecimal * (j + 1);

        result[0] = result[1] = result[2] = 0.0f;
        for (int jj = 0; jj < hSupport; jj++) {
          uint8_to_float(input, prevLevelMem + prevLevelPitch * (2 * j + jj));
          result[0] += hWeight[jj] * input[0];
          result[1] += hWeight[jj] * input[1];
          result[2] += hWeight[jj] * input[2];
        }

        // convert back to format of the texture
        float_to_uint8(currLevelMem + (currLevelPitch * j), result);
      }

      // case 3: reduction in both horizontal and vertical size
    } else {

      for (int j = 0; j < currLevel.height; j++) {
        hWeight[0] = hNorm * (1.0f - hDecimal * j);
        hWeight[1] = hNorm;
        hWeight[2] = hNorm * hDecimal * (j + 1);

        for (int i = 0; i < currLevel.width; i++) {
          wWeight[0] = wNorm * (1.0f - wDecimal * i);
          wWeight[1] = wNorm * 1.0f;
          wWeight[2] = wNorm * wDecimal * (i + 1);

          result[0] = result[1] = result[2] = 0.0f;

          // convolve source image with a trapezoidal filter.
          // in the case of no rounding this is just a box filter of width 2.
          // in the general case, the support region is 3x3.
          for (int jj = 0; jj < hSupport; jj++)
            for (int ii = 0; ii < wSupport; ii++) {
              float weight = hWeight[jj] * wWeight[ii];
              uint8_to_float(input, prevLevelMem +
                                        prevLevelPitch * (2 * j + jj) +
                                        3 * (2 * i + ii));
              result[0] += weight * input[0];
              result[1] += weight * input[1];
              result[2] += weight * input[2];
            }

          // convert back to format of the texture
          float_to_uint8(currLevelMem + currLevelPitch * j + 3 * i, result);
        }
      }
    }
  }
}

}
