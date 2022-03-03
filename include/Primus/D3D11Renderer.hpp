#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Primus/FrameState.hpp"

class TaskScheduler;

class D3D11Renderer {
public:

  explicit D3D11Renderer(HWND window, TaskScheduler&);
  D3D11Renderer(const D3D11Renderer& other) = delete;
  D3D11Renderer(const D3D11Renderer&& other) = delete;
  ~D3D11Renderer() = default;

  void onWindowResize(int clientAreaWidth, int clientAreaHeight);

  void render(const FrameState& frameState);
};
