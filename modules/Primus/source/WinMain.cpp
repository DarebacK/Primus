#define DAR_MODULE_NAME "WinMain"

#include <stdio.h>

#include "Core/Core.hpp"
#include "Core/Math.hpp"
#include "Core/Task.hpp"
#include "Core/Asset.hpp"
#include "Core/File.hpp"
#include "Core/WindowsPlatform.h"

#include <windowsx.h>
#include <Psapi.h>

#include "Primus/Game.hpp"
#include "Primus/D3D11Renderer.hpp"

#define VK_Q 0x51
#define VK_W 0x57
#define VK_E 0x45
#define VK_A 0x41
#define VK_S 0x53
#define VK_D 0x44

namespace
{
  class Frames
  {
  public:
    Frame* getLast(unsigned int frameIndex) { return frames + (--frameIndex % size); };  // overflow when frameIndex == 0 shouldn't be a problem
    Frame* getNext(unsigned int frameIndex) { return frames + (frameIndex % size); };

  private:
    static constexpr int size = 2;
    Frame frames[size] = {};
  };

  int clientAreaWidth = GetSystemMetrics(SM_CXSCREEN);
  int clientAreaHeight = GetSystemMetrics(SM_CYSCREEN);
  MainWindow window;
  HANDLE process = nullptr;
  WINDOWPLACEMENT windowPosition = { sizeof(windowPosition) };
  const TCHAR* gameName = L"Primus";
  int coreCount = 0;
  Frames frames;
  Frame* lastFrame = nullptr;
  Frame* nextFrame = nullptr;
  int frameCount = 0;

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
        if (nextFrame->input.processMessage(windowHandle, message, wParam, lParam))
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
}

static void debugShowResourcesUsage()
{
#ifdef DAR_DEBUG
  // from https://stackoverflow.com/a/64166/9178254

  // CPU
  static ULARGE_INTEGER lastCPU = {}, lastSysCPU = {}, lastUserCPU = {};
  static double percent = 0.;
  static ULONGLONG lastCheckTimeMs = 0;
  ULONGLONG currentTimeMs = GetTickCount64();
  if ((currentTimeMs - lastCheckTimeMs) > 1000) {
    ULARGE_INTEGER now, sys, user;
    GetSystemTimeAsFileTime((LPFILETIME)&now);
    FILETIME fileTime;
    GetProcessTimes(process, &fileTime, &fileTime, (LPFILETIME)&sys, (LPFILETIME)&user);
    percent = double((sys.QuadPart - lastSysCPU.QuadPart) +
      (user.QuadPart - lastUserCPU.QuadPart));
    percent /= double(now.QuadPart - lastCPU.QuadPart);
    percent /= double(coreCount);
    percent *= 100;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;
    lastCheckTimeMs = currentTimeMs;
  }
  debugText(L"CPU %.f%%", percent);

  // Memory
  MEMORYSTATUSEX memoryStatus;
  memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memoryStatus);
  DWORDLONG totalVirtualMemoryMB = memoryStatus.ullTotalPageFile / (1ull << 20ull);
  DWORDLONG totalPhysicalMemoryMB = memoryStatus.ullTotalPhys / (1ull << 20ull);
  PROCESS_MEMORY_COUNTERS_EX processMemoryCounters;
  GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&processMemoryCounters, sizeof(processMemoryCounters));
  SIZE_T virtualMemoryUsedByGameMB = processMemoryCounters.PrivateUsage / (1ull << 20ull);
  SIZE_T physicalMemoryUsedByGameMB = processMemoryCounters.WorkingSetSize / (1ull << 20ull);
  debugText(L"Virtual memory %llu MB / %llu MB", virtualMemoryUsedByGameMB, totalVirtualMemoryMB);
  debugText(L"Physical memory %llu MB / %llu MB", physicalMemoryUsedByGameMB, totalPhysicalMemoryMB);

#endif
}

int WINAPI WinMain(
  HINSTANCE instanceHandle,
  HINSTANCE hPrevInstance, // always zero
  LPSTR commandLine,
  int showCode
)
{
  // TODO: do only in profile configuration
  TRACE_START_CAPTURE();

  process = GetCurrentProcess();

  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  coreCount = systemInfo.dwNumberOfProcessors;

  window.tryInitializeGameStyle(instanceHandle, gameName, &WindowProc);

  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
  {
    logWarning("Failed to set highest thread priority for the main thread.");
  }

  TaskManagerGuard taskManagerGuard;

  FileSystemGuard fileSystemGuard;

  D3D11Renderer renderer;
  if (!renderer.tryInitialize(window)) {
    window.showErrorMessageBox(L"Failed to initialize Direct3D11 renderer", L"Fatal error");
    return -1;
  }

  //Audio audio;

  Game game;

  window.show();

  lastFrame = frames.getLast(frameCount);
  lastFrame->input.cursorPosition = window.getCursorPosition();
  lastFrame->clientAreaWidth = clientAreaWidth;
  lastFrame->clientAreaHeight = clientAreaHeight;
  lastFrame->aspectRatio = float(clientAreaWidth) / clientAreaHeight;

  nextFrame = frames.getNext(frameCount);

  if (!game.tryInitialize(*lastFrame, renderer))
  {
    window.showErrorMessageBox(L"Failed to initialize game.", L"Fatal error");
    return -1;
  }

  runGameLoop([&](int64 frameIndex, float timeDelta) {
    nextFrame->input.cursorPosition = window.getCursorPosition();
    nextFrame->clientAreaWidth = clientAreaWidth;
    nextFrame->clientAreaHeight = clientAreaHeight;
    nextFrame->aspectRatio = float(clientAreaWidth) / clientAreaHeight;

    nextFrame->deltaTime = timeDelta;

    debugResetText();
    debugText(L"%.3f s / %d fps", nextFrame->deltaTime, (int)(1.f / nextFrame->deltaTime));
    debugShowResourcesUsage();

    taskManager.processMainTasks();

    game.update(*lastFrame, *nextFrame, renderer);

    if (lastFrame->clientAreaWidth != clientAreaWidth || lastFrame->clientAreaHeight != clientAreaHeight) {
      renderer.onWindowResize(clientAreaWidth, clientAreaHeight);
    }

    //audio.update(*nextFrameState);

    lastFrame = frames.getLast(frameIndex);
    nextFrame = frames.getNext(frameIndex);
    nextFrame->input = lastFrame->input;
    nextFrame->input.resetForNextFrame();
  });

  // TODO: ensure the folder is created
  // TODO: stop when actually loaded
  TRACE_STOP_CAPTURE("profiling\\GameLoad.opt");

  return 0;
}