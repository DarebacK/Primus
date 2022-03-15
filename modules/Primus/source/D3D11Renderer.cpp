#define DAR_MODULE_NAME "D3D11Renderer"

#include "Primus/D3D11Renderer.hpp"
#include "Primus/ShaderRegistry.hpp"

#include "Core/Task.hpp"

#include <vector>

#include <d3d11_4.h>
#include <dwrite_2.h>
#include <d2d1_2.h>

#include "D3D11/ShaderRegistryImpl.inl"
#include "IndexBuffers.inl"
#include "ConstantBuffers.inl"

using namespace D3D11;

namespace D3D11
{
  CComPtr<ID3D11Device> device;
  CComPtr<ID3D11DeviceContext> context = nullptr;
}

namespace
{
  D3D_FEATURE_LEVEL featureLevel = (D3D_FEATURE_LEVEL)0;
  CComPtr<IDXGIAdapter3> dxgiAdapter;
  DXGI_ADAPTER_DESC2 dxgiAdapterDesc{};
  CComPtr<IDXGISwapChain1> swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  constexpr UINT msaaSampleCount = 8;
  CComPtr<ID3D11Texture2D> renderTarget = nullptr;
  CD3D11_TEXTURE2D_DESC renderTargetDesc = {};
  CComPtr<ID3D11RenderTargetView> renderTargetView = nullptr;
  CComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;
  float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
  CComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
  D3D11_RASTERIZER_DESC rasterizerDesc =
  {
    D3D11_FILL_SOLID,
    D3D11_CULL_BACK,
    FALSE, // FrontCounterClockwise
    D3D11_DEFAULT_DEPTH_BIAS,
    D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE, // DepthClipEnable
    FALSE, // ScissorEnable
    TRUE, // MultisampleEnable
    TRUE // AntialiasedLineEnable
  };
  CComPtr<IDWriteFactory2> dwriteFactory;
  CComPtr<ID2D1Device1> d2Device = nullptr;
  CComPtr<ID2D1DeviceContext1> d2Context = nullptr;
  constexpr float verticalFieldOfView = 74.f;
  constexpr float nearPlane = 1.f;
  constexpr float farPlane = 100.f;
  Mat4f projectionMatrix = Mat4f::identity();

#ifdef DAR_DEBUG
  CComPtr<ID3D11Debug> debug = nullptr;
  CComPtr<ID2D1SolidColorBrush> debugTextBrush = nullptr;
  CComPtr<IDWriteTextFormat> debugTextFormat = nullptr;
#endif

  static void setViewport(FLOAT width, FLOAT height)
  {
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
  }

  static void updateViewport()
  {
    setViewport((FLOAT)renderTargetDesc.Width, (FLOAT)renderTargetDesc.Height);

    float aspectRatio = (float)renderTargetDesc.Width / renderTargetDesc.Height;
    projectionMatrix = Mat4f::perspectiveProjectionD3d(
      degreesToRadians(verticalFieldOfView),
      aspectRatio,
      nearPlane,
      farPlane
    );
  }

  static CComPtr<ID3D11Texture2D> createRenderTarget()
  {
    renderTargetDesc = CD3D11_TEXTURE2D_DESC(
      swapChainDesc.Format,
      swapChainDesc.Width,
      swapChainDesc.Height,
      1,
      1,
      D3D11_BIND_RENDER_TARGET,
      D3D11_USAGE_DEFAULT,
      0,
      msaaSampleCount
    );

    CComPtr<ID3D11Texture2D> result = nullptr;
    if (FAILED((device->CreateTexture2D(&renderTargetDesc, nullptr, &result)))) {
      logError("Failed to create render target texture.");
      return nullptr;
    }
    return result;
  }

  static CComPtr<ID3D11RenderTargetView> createRenderTargetView(ID3D11Resource* renderTarget, DXGI_FORMAT format)
  {
    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, format);
    CComPtr<ID3D11RenderTargetView> result = nullptr;
    if (FAILED(device->CreateRenderTargetView(renderTarget, &renderTargetViewDesc, &result))) {
      logError("Failed to create render target view.");
      return nullptr;
    }
    return result;
  }

  static CComPtr<ID3D11DepthStencilView> createDepthStencilView()
  {
    D3D11_TEXTURE2D_DESC depthStencilDesc{};
    depthStencilDesc.Width = renderTargetDesc.Width;
    depthStencilDesc.Height = renderTargetDesc.Height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.SampleDesc.Count = renderTargetDesc.SampleDesc.Count;
    depthStencilDesc.SampleDesc.Quality = renderTargetDesc.SampleDesc.Quality;
    CComPtr<ID3D11Texture2D> depthStencilBuffer = nullptr;
    if (FAILED(device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer))) {
      logError("Failed to create depth stencil buffer");
      return nullptr;
    }

    CComPtr<ID3D11DepthStencilView> result = nullptr;
    if (FAILED(device->CreateDepthStencilView(depthStencilBuffer, nullptr, &result))) {
      logError("Failed to create depth stencil view");
      return nullptr;
    }
    return result;
  }

  static bool bindD2dTargetToD3dTarget()
  {
    CComPtr<IDXGISurface> dxgiBackBuffer;
    if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))))
    {
      logError("Failed to get swapchains IDXGISurface back buffer.");
      return false;
    }
    D2D1_BITMAP_PROPERTIES1 d2BitmapProperties{};
    d2BitmapProperties.pixelFormat.format = swapChainDesc.Format;
    d2BitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    d2BitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    CComPtr<ID2D1Bitmap1> d2Bitmap;
    if (FAILED(d2Context->CreateBitmapFromDxgiSurface(
      dxgiBackBuffer,
      &d2BitmapProperties,
      &d2Bitmap
    ))) {
      logError("Failed to create ID2D1Bitmap render target.");
      return false;
    }
    d2Context->SetTarget(d2Bitmap);
    return true;
  }

} // anonymous namespace

static void initializeTerrain()
{
  if (FAILED(device->CreateBuffer(&terrainIndexBufferDescription, &terrainIndexBufferData, &terrainIndexBuffer)))
  {
    logError("Failed to create terrain index buffer.");
    return;
  }

  if (FAILED(device->CreateBuffer(&terrainConstantBufferDescription, nullptr, &terrainConstantBuffer)))
  {
    logError("Failed to create terrain constant buffer.");
    return;
  }
}

bool D3D11Renderer::tryInitialize(HWND window)
{
  // DEVICE
  UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DAR_DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
  if (FAILED(D3D11CreateDevice(
    NULL,
    D3D_DRIVER_TYPE_HARDWARE,
    NULL,
    createDeviceFlags,
    featureLevels,
    arrayLength(featureLevels),
    D3D11_SDK_VERSION,
    &device,
    &featureLevel,
    &context
  ))) {
    logError("Failed to create D3D11 device");
    return false;
  }

#ifdef DAR_DEBUG
  device->QueryInterface(IID_PPV_ARGS(&debug));
#endif

  // SWAPCHAIN
  CComPtr<IDXGIDevice> dxgiDevice;
  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
    if (SUCCEEDED(dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter)))) {
      CComPtr<IDXGIFactory2> dxgiFactory;
      if (SUCCEEDED(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)))) {
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;
        dxgiFactory->CreateSwapChainForHwnd(dxgiDevice, window, &swapChainDesc, NULL, NULL, &swapChain);
        if (!swapChain) {
          // DXGI_SWAP_EFFECT_FLIP_DISCARD may not be supported on this OS. Try legacy DXGI_SWAP_EFFECT_DISCARD.
          swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
          dxgiFactory->CreateSwapChainForHwnd(dxgiDevice, window, &swapChainDesc, NULL, NULL, &swapChain);
          if (!swapChain) {
            logError("Failed to create swapChain.");
            return false;
          }
        }
        swapChain->GetDesc1(&swapChainDesc);
        if (FAILED(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER))) {
          logError("Failed to make window association to ignore alt enter.");
        }
      }
      else {
        logError("Failed to get IDXGIFactory");
        return false;
      }

      if (FAILED(dxgiAdapter->GetDesc2(&dxgiAdapterDesc))) {
        logError("Failed to get adapter description.");
      }
    }
    else {
      logError("Failed to get IDXGIAdapter.");
      return false;
    }

    // DirectWrite and Direct2D
    if (SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), (IUnknown**)&dwriteFactory))) {
      D2D1_FACTORY_OPTIONS options;
#ifdef DAR_DEBUG
      options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
      options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif
      CComPtr<ID2D1Factory2> d2dFactory;
      if (SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), &options, (void**)&d2dFactory))) {
        if (FAILED(d2dFactory->CreateDevice(dxgiDevice, &d2Device))) {
          logError("Failed to create ID2D1Device");
        }
        if (FAILED(d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2Context))) {
          logError("Failed to create ID2D1DeviceContext");
        }
      }
    }
  }
  else {
    logError("Failed to get IDXGIDevice.");
    return false;
  }

  renderTarget = createRenderTarget();
  if (!renderTarget) {
    logError("Failed to create render target.");
    return false;
  }

  renderTargetView = createRenderTargetView(renderTarget, renderTargetDesc.Format);
  if (!renderTargetView) {
    logError("Failed to initialize render target view.");
    return false;
  }
  depthStencilView = createDepthStencilView();
  if (!depthStencilView) {
    logError("Failed to initialize depth stencil view.");
    return false;
  }
  context->OMSetRenderTargets(1, &renderTargetView.p, depthStencilView);

  updateViewport();

  device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
  context->RSSetState(rasterizerState);

  bindD2dTargetToD3dTarget();

#ifdef DAR_DEBUG
  // DEBUG TEXT
  d2Context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &debugTextBrush);
  dwriteFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12, L"en-US", &debugTextFormat);
  debugTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
  debugTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
#endif

  reloadShaders(L"shaders\\build");

  initializeTerrain();

  return true;
}

void D3D11Renderer::onWindowResize(int clientAreaWidth, int clientAreaHeight)
{
  if (swapChain) {
    d2Context->SetTarget(nullptr);  // Clears the binding to swapChain's back buffer.
    depthStencilView.Release();
    renderTargetView.Release();
    renderTarget.Release();
    if (FAILED(swapChain->ResizeBuffers(0, 0, 0, swapChainDesc.Format, swapChainDesc.Flags))) {
#ifdef DAR_DEBUG
      debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_SUMMARY);
#endif DAR_DEBUG
      logError("Failed to resize swapChain buffers");
      return;
    }
    if (FAILED(swapChain->GetDesc1(&swapChainDesc))) {
      logError("Failed to get swapChain desc after window resize");
      return;
    }

    renderTarget = createRenderTarget();
    if (!renderTarget) {
      logError("Failed to create render target after window resize.");
      return;
    }

    renderTargetView = createRenderTargetView(renderTarget, renderTargetDesc.Format);
    if (!renderTargetView) {
      logError("Failed to create render target view after window resize.");
      return;
    }
    depthStencilView = createDepthStencilView();
    if (!depthStencilView) {
      logError("Failed to create depth stencil view after window resize.");
      return;
    }
    context->OMSetRenderTargets(1, &renderTargetView.p, depthStencilView);

    if (!bindD2dTargetToD3dTarget()) {
      logError("Failed to bind Direct2D render target to Direct3D render target after window resize.");
      return;
    }

    updateViewport();
  }
}

static void switchWireframeState()
{
#ifdef DAR_DEBUG
  rasterizerDesc.FillMode = rasterizerDesc.FillMode == D3D11_FILL_SOLID ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
  rasterizerState.Release();
  device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
  context->RSSetState(rasterizerState);
#endif
}

static void resolveRenderTargetIntoBackBuffer()
{
  CComPtr<ID3D11Texture2D> backBuffer = nullptr;
  if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
    context->ResolveSubresource(backBuffer, 0, renderTarget, 0, swapChainDesc.Format);
  }
  else {
    logError("resolveRenderTargetIntoBackBuffer failed. Failed to get swapChain's back buffer.");
  }
}

static void displayVideoMemoryInfo()
{
#if defined(DAR_DEBUG)
  DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo{};
  UINT64 videoMemoryUsageMB = 0;
  if (SUCCEEDED(dxgiAdapter->QueryVideoMemoryInfo(
    0,
    DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
    &videoMemoryInfo
  ))) {
    videoMemoryUsageMB = videoMemoryInfo.CurrentUsage / (1ull << 20ull);
  }
  else {
    logError("Failed to query video memory info.");
  }
  UINT64 videoMemoryTotalMB = dxgiAdapterDesc.DedicatedVideoMemory / (1ull << 20ull);
  debugText(L"VRAM %llu MB / %llu MB", videoMemoryUsageMB, videoMemoryTotalMB);
#endif 
}

static void drawDebugText(const Frame& frameState)
{
  #ifdef DAR_DEBUG
    // DEBUG TEXT
    CComPtr<IDWriteTextLayout> debugTextLayout;
    dwriteFactory->CreateTextLayout(
      _debugText,
      _debugTextLength,
      debugTextFormat,
      (FLOAT)frameState.clientAreaWidth,
      (FLOAT)frameState.clientAreaHeight,
      &debugTextLayout
    );
    d2Context->DrawTextLayout({ 5.f, 5.f }, debugTextLayout, debugTextBrush);
  #endif
}

static void renderTerrain(const Frame& frame)
{
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->IASetInputLayout(FullScreenInputLayout);
  context->IASetIndexBuffer(terrainIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  context->VSSetShader(TerrainVertexShader, nullptr, 0);
  context->VSSetConstantBuffers(0, 1, &terrainConstantBuffer.p);
  context->PSSetShader(TerrainPixelShader, nullptr, 0);

  D3D11_MAPPED_SUBRESOURCE mappedConstantBuffer;
  context->Map(terrainConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedConstantBuffer);
  const Mat4f transform = frame.camera.viewProjection;
  memcpy(mappedConstantBuffer.pData, &transform, sizeof(transform));
  context->Unmap(terrainConstantBuffer, 0);

  context->DrawIndexed(arrayLength(terrainIndexBufferIndices), 0, 0);
}

static void render3D(const Frame& frameState)
{
  renderTerrain(frameState);

  resolveRenderTargetIntoBackBuffer();
}

static void render2D(const Frame& frameState)
{
  d2Context->BeginDraw();

  drawDebugText(frameState);

  d2Context->EndDraw();
}

void D3D11Renderer::render(const Frame& frameState)
{
  
#ifdef DAR_DEBUG
  if (frameState.input.keyboard.F1.pressedDown) {
    switchWireframeState();
  }
  if (frameState.input.keyboard.F5.pressedDown) {
    reloadShaders(L"shaders\\build");
  }

  displayVideoMemoryInfo();
#endif

  context->ClearRenderTargetView(renderTargetView, clearColor);
  context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  render3D(frameState);

  render2D(frameState);

  UINT presentFlags = 0;
  swapChain->Present(1, presentFlags);
}
