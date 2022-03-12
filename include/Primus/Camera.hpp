#pragma once

#include "Core/Math.hpp"

struct Camera
{
  Vec3f position = Vec3f{ 0.f, 10.f, 0.f };
  Mat4x3f view;
  Mat4f projection;
  Mat4f viewProjection;
};