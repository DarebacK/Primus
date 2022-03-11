#pragma once

#include "Primus/Frame.hpp"

class Game
{
public:

  void initialize(Frame& firstFrame);

  void update(const Frame& lastFrame, Frame& nextFrame);
};