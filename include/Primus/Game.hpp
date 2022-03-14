#pragma once

#include "Primus/Frame.hpp"

class Game
{
public:

  bool tryInitialize(Frame& firstFrame);

  void update(const Frame& lastFrame, Frame& nextFrame);
};