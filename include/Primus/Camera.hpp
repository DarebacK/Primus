#pragma once

#include "Core/Math.hpp"

struct Camera
{
  Vec3f currentPosition;
  Vec3f endPosition; // The target position the camera interpolates to.
  float zoomMin = 0.f;
  float zoomMax = 0.f;
  float nearPlane = 0.f;
  float farPlane = 0.f;
  Mat4x3f view;
  Mat4f projection;
  Mat4f viewProjection;
  Mat4f viewProjectionInverse;
  SimpleViewFrustum frustum;
};