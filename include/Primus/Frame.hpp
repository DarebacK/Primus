#pragma once

#include "Core/Input.hpp"

#include "Primus/Camera.hpp"

struct Map;

// Holds game state with lifetime of a frame.
struct Frame
{
  Input input;

  Camera camera;

  float deltaTime;

  int clientAreaWidth;
  int clientAreaHeight;
  float aspectRatio;

  void updateCameraZoomLimits(const Map& currentMap);
  void updateCamera(const Map& currentMap, const Vec3f& currentPosition, const Vec3f& endPosition);
};