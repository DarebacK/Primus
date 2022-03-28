#pragma once
#include "Core/Core.hpp"

#include "Primus/Frame.hpp"
#include "Primus/Map.hpp"

class TaskScheduler;

class D3D11Renderer {
public:

  D3D11Renderer() = default;
  D3D11Renderer(const D3D11Renderer& other) = delete;
  D3D11Renderer(const D3D11Renderer&& other) = delete;
  ~D3D11Renderer() = default;

  bool tryInitialize(HWND window);

  void onWindowResize(int clientAreaWidth, int clientAreaHeight);

  bool tryLoadMap(const Map& map);

  void render(const Frame& frameState, const Map& map);
};
