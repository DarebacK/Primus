#pragma once

#include "Core/Input.hpp"

#include "Primus/Camera.hpp"

// Holds game state with lifetime of a frame.
struct Frame
{
  Input input;

  Camera camera;

  float deltaTime;

  int clientAreaWidth;
  int clientAreaHeight;
};