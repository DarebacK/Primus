#pragma once

#include "Core/Math.hpp"

struct Camera
{
  Vec3f currentPosition;
  Vec3f endPosition; // The target position the camera interpolates to.
  Mat4x3f view;
  Mat4f projection;
  Mat4f viewProjection;
  Mat4f viewProjectionInverse;
  SimpleViewFrustum frustum;
};