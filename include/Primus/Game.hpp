#pragma once

#include "Primus/Frame.hpp"

class Game
{
public:

  void update(const Frame& lastFrame, Frame& nextFrame);
};