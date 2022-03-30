#define NOMINMAX

#include "Core/Core.hpp"
#include "Core/Task.hpp"
#include "Core/Image.hpp"
#include "Core/WindowsPlatform.h"

#include <vector>
#include <memory>
#include <fstream>

#include "Internet.hpp"
#include "Heightmap.hpp"
#include "Colormap.hpp"

MainWindow window;

LRESULT CALLBACK WindowProc(
  HWND   windowHandle,
  UINT   message,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch (message) {
    case WM_SIZE: {
      int newClientAreaWidth = LOWORD(lParam);
      int newClientAreaHeight = HIWORD(lParam);
      return 0;
    }
    default:
    {
      return DefWindowProc(windowHandle, message, wParam, lParam);
    }
  }
}

int WINAPI WinMain(
  HINSTANCE instanceHandle,
  HINSTANCE hPrevInstance, // always zero
  LPSTR commandLine,
  int showCode
)
{
  taskScheduler.initialize();

  if (!window.tryInitialize(instanceHandle, L"PrimusEditor", &WindowProc))
  {
    return -1;
  }
  window.show();

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