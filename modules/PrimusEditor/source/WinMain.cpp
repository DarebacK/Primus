#define NOMINMAX

#include "Core/Core.hpp"
#include "Core/Task.hpp"
#include "Core/Image.hpp"
#include "Core/WindowsPlatform.h"

#include "D3D11/D3D11.hpp"

#include "ImGui/ImGui.hpp"

#include "Primus/D3D11Renderer.hpp"

#include <vector>
#include <memory>
#include <fstream>

#include "Internet.hpp"
#include "Heightmap.hpp"
#include "Colormap.hpp"

MainWindow window;

Input input;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(
  HWND   windowHandle,
  UINT   message,
  WPARAM wParam,
  LPARAM lParam
)
{
  if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
  {
    return 0;
  }

  switch (message) {
    case WM_SIZE: {
      int newClientAreaWidth = LOWORD(lParam);
      int newClientAreaHeight = HIWORD(lParam);
      return 0;
    }
    case WM_CLOSE: {
      PostQuitMessage(0);
      return DefWindowProc(windowHandle, message, wParam, lParam);
    }
    default:
    {
      if (input.processMessage(windowHandle, message, wParam, lParam))
      {
        return 0;
      }
      else
      {
        return DefWindowProc(windowHandle, message, wParam, lParam);
      }
    }
  }
}

struct DownloadPopup
{
  ImGuiID id = 0;

  int tileRangeTopLeft[2] = { 1, 1 };
  int tileRangeBottomRight[2] = { 1, 1 };
  int tileRangeZoom = 1;
  int heightmapZoom = 1;
  int colormapZoom = 1;

  void open()
  {
    ImGui::OpenPopup(id);
  }

  void define()
  {
    if (ImGui::BeginPopupModal("Download", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::InputInt("Zoom", &tileRangeZoom, 1, 2);
      ImGui::InputInt2("Top left", tileRangeTopLeft);
      ImGui::InputInt2("Bottom right", tileRangeBottomRight);

      ImGui::Separator();

      ImGui::InputInt("Heightmap zoom", &heightmapZoom, 1, 2);
      ImGui::InputInt("Colormap zoom", &colormapZoom, 1, 2);

      ImGui::Separator();

      if (ImGui::Button("Download"))
      {
        // TODO: download
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

} downloadPopup;

static void defineGui()
{
  downloadPopup.id = ImGui::GetID("Download");

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Map"))
    {
      if (ImGui::MenuItem("Open"))
      {

      }
      if (ImGui::MenuItem("Download"))
      {
        downloadPopup.open();
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  ImGui::ShowDemoWindow();

  downloadPopup.define();
}

int WINAPI WinMain(
  HINSTANCE instanceHandle,
  HINSTANCE hPrevInstance, // always zero
  LPSTR commandLine,
  int showCode
)
{
  taskScheduler.initialize();

  if (!window.tryInitializeEditorStyle(instanceHandle, L"PrimusEditor", &WindowProc))
  {
    return -1;
  }

  D3D11Renderer renderer;
  if (!renderer.tryInitialize(window))
  {
    window.showErrorMessageBox(L"Failed to initialize D3D11Renderer.", L"Fatal Error");
    return -2;
  }

  Dar::ImGui imGui{ window, D3D11::device, D3D11::context };  

  ImGuiIO& imGuiIo = ImGui::GetIO();
  imGuiIo.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\LeelawUI.ttf", 16);

  window.show();

  runGameLoop([&](int64 frameIndex, float timeDelta) {
    imGui.newFrame();

    defineGui();

    renderer.beginRender();

    renderer.setBackBufferRenderTarget();
    imGui.render();

    renderer.endRender();
  });

  return 0;
}

//int main(int argc, char* argv[])
//{
//  taskScheduler.initialize();
//
//  InternetHandle internet{ InternetOpen(L"Primus MapTool", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0) };
//  if (!internet)
//  {
//    logError("InternetOpen failed.");
//    return 1;
//  }
//
//  //downloadHeightmap(internet.get(), "heightmap.png");
//  downloadColormap(internet.get(), "colormap.png");
//
//  return 0;
//}