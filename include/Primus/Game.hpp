#pragma once

#include "Primus/Frame.hpp"
#include "Primus/D3D11Renderer.hpp"

class Game
{
public:

  bool tryInitialize(Frame& firstFrame, D3D11Renderer& renderer);

  void update(const Frame& lastFrame, Frame& nextFrame, D3D11Renderer& renderer);
};