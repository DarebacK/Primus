#define DAR_MODULE_NAME "WinMain"

#include <stdio.h>

#include "Core/Core.hpp"
#include "Core/Math.hpp"
#include "Core/Task.hpp"

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
  HWND window = nullptr;
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

static void showErrorMessageBox(const wchar_t* text, const wchar_t* caption)
{
  MessageBox(window, text, caption, MB_OK | MB_ICONERROR);
}

static Vec2i getCursorPosition()
{
  Vec2i mousePosition;
  if (!GetCursorPos((LPPOINT)&mousePosition)) {
    return {};
  }
  if (!ScreenToClient(window, (LPPOINT)&mousePosition)) {
    return {};
  }
  return mousePosition;
}

int WINAPI WinMain(
  HINSTANCE instanceHandle,
  HINSTANCE hPrevInstance, // always zero
  LPSTR commandLine,
  int showCode
)
{
  process = GetCurrentProcess();

  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  coreCount = systemInfo.dwNumberOfProcessors;

  WNDCLASS windowClass{};
  windowClass.lpfnWndProc = &WindowProc;
  windowClass.hInstance = instanceHandle;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = L"Game window class";
  if (!RegisterClass(&windowClass)) {
    showErrorMessageBox(L"Failed to register window class.", L"Fatal error");
    return -1;
  }

  RECT windowRectangle = { 0, 0, clientAreaWidth, clientAreaHeight };
  constexpr DWORD windowStyle = WS_POPUP;
  constexpr DWORD windowStyleEx = 0;
  AdjustWindowRectEx(&windowRectangle, windowStyle, false, windowStyleEx);
  const int windowWidth = windowRectangle.right - windowRectangle.left;
  const int windowHeight = windowRectangle.bottom - windowRectangle.top;
  window = CreateWindowEx(
    windowStyleEx,
    windowClass.lpszClassName,
    gameName,
    windowStyle,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    windowWidth,
    windowHeight,
    nullptr,
    nullptr,
    windowClass.hInstance,
    nullptr
  );
  if (!window) {
    showErrorMessageBox(L"Failed to create game window", L"Fatal error");
    return -1;
  }

  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
  {
    logWarning("Failed to set highest thread priority for the main thread.");
  }

  if (SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1)) == 0)
  {
    logWarning("Failed to set thread affinity for the main thread.");
  }

  taskScheduler.initialize();

  D3D11Renderer renderer;
  if (!renderer.tryInitialize(window)) {
    showErrorMessageBox(L"Failed to initialize Direct3D11 renderer", L"Fatal error");
    return -1;
  }

  //Audio audio;

  Game game;

  ShowWindow(window, SW_SHOWNORMAL);

  lastFrame = frames.getLast(frameCount);
  lastFrame->input.cursorPosition = getCursorPosition();
  lastFrame->clientAreaWidth = clientAreaWidth;
  lastFrame->clientAreaHeight = clientAreaHeight;
  lastFrame->aspectRatio = float(clientAreaWidth) / clientAreaHeight;

  nextFrame = frames.getNext(frameCount);

  LARGE_INTEGER counterFrequency;
  QueryPerformanceFrequency(&counterFrequency);
  LARGE_INTEGER lastCounterValue;
  QueryPerformanceCounter(&lastCounterValue);

  if (!game.tryInitialize(*lastFrame, renderer))
  {
    showErrorMessageBox(L"Failed to initialize game.", L"Fatal error");
    return -1;
  }

  MSG message{};
  while (message.message != WM_QUIT) {
    if (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageA(&message);
    }
    else {
      // process frame

      nextFrame->input.cursorPosition = getCursorPosition();
      nextFrame->clientAreaWidth = clientAreaWidth;
      nextFrame->clientAreaHeight = clientAreaHeight;
      nextFrame->aspectRatio = float(clientAreaWidth) / clientAreaHeight;

      LARGE_INTEGER currentCounterValue;
      QueryPerformanceCounter(&currentCounterValue);
      nextFrame->deltaTime = (float)(currentCounterValue.QuadPart - lastCounterValue.QuadPart) / counterFrequency.QuadPart;
      lastCounterValue = currentCounterValue;

      debugResetText();
      debugText(L"%.3f s / %d fps", nextFrame->deltaTime, (int)(1.f / nextFrame->deltaTime));
      debugShowResourcesUsage();

      game.update(*lastFrame, *nextFrame, renderer);

      if (lastFrame->clientAreaWidth != clientAreaWidth || lastFrame->clientAreaHeight != clientAreaHeight) {
          renderer.onWindowResize(clientAreaWidth, clientAreaHeight);
      }

      //audio.update(*nextFrameState);

      ++frameCount;

      lastFrame = frames.getLast(frameCount);
      nextFrame = frames.getNext(frameCount);
      nextFrame->input = lastFrame->input;
      nextFrame->input.resetForNextFrame();
      /*nextFrame->input.keyboard = {};
      nextFrame->input.mouse.dWheel = 0.f;*/
    }
  }
  return 0;
}