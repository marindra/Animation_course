#pragma once

struct AnimationInfo
{
  unsigned int rawSize = 0;
  unsigned int finalSize = 0;
  unsigned int optimizedSize = 0;

  bool needToOptimize = false;

  float tolerance = 1e-3;
  float distance = 1e-1;
};