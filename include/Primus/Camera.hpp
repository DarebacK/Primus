#pragma once

#include "Core/Math.hpp"

struct Camera
{
  Vec3f position;
  Mat4x3f view;
  Mat4f projection;
  Mat4f viewProjection;
};