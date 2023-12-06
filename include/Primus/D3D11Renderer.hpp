#pragma once
#include "Core/Core.hpp"

#include "Primus/Frame.hpp"
#include "Primus/Map.hpp"

class D3D11Renderer {
public:

  D3D11Renderer() = default;
  D3D11Renderer(const D3D11Renderer& other) = delete;
  D3D11Renderer(const D3D11Renderer&& other) = delete;
  ~D3D11Renderer() = default;

  bool tryInitialize(HWND window, bool bIsEditor);

  void onWindowResize(const Vec2i& clientArea);
  Vec2i getViewportSize() const;
  void setViewportSize(const Vec2i& size);

  bool tryLoadMap(const Map& map);

  void beginRender();

  void setMainRenderTarget();
  // Only in editor
  void* getMainRenderTargetSrv();

  void render(const Frame& frameState, const Map& map);

  void setBackBufferRenderTarget();

  void endRender();
};
