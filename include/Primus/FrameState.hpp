#pragma once

#include "Core/Input.hpp"

// Holds game state with lifetime of a frame.
struct FrameState
{
  Input input;

  float deltaTime;

  int clientAreaWidth;
  int clientAreaHeight;
};