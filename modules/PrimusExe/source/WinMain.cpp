#define DAR_MODULE_NAME "WinMain"

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <Psapi.h>

#include "Core/Core.hpp"
#include "Core/Math.hpp"
#include "Core/TaskScheduler.hpp"

#include "Primus/D3D11Renderer.hpp"

#define VK_Q 0x51
#define VK_W 0x57
#define VK_E 0x45
#define VK_A 0x41
#define VK_S 0x53
#define VK_D 0x44

namespace
{
  class FrameStates
  {
  public:
    FrameState* getLastState(unsigned int frameIndex) { return states + (--frameIndex % size); };  // overflow when frameIndex == 0 shouldn't be a problem
    FrameState* getNextState(unsigned int frameIndex) { return states + (frameIndex % size); };

  private:
    static constexpr int size = 2;
    FrameState states[size] = {};
  };

  int clientAreaWidth = GetSystemMetrics(SM_CXSCREEN);
  int clientAreaHeight = GetSystemMetrics(SM_CYSCREEN);
  HWND window = nullptr;
  HANDLE process = nullptr;
  WINDOWPLACEMENT windowPosition = { sizeof(windowPosition) };
  const TCHAR* gameName = L"Primus";
  int coreCount = 0;
  FrameStates frameStates;
  FrameState* lastFrameState = nullptr;
  FrameState* nextFrameState = nullptr;
  int frameCount = 0;

  TaskScheduler taskScheduler;

  LRESULT CALLBACK WindowProc(
    HWND   windowHandle,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
  )
  {
    LRESULT result = 0;
    switch (message) {
    case WM_SIZE: {
      int newClientAreaWidth = LOWORD(lParam);
      int newClientAreaHeight = HIWORD(lParam);

    }
                break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_LBUTTONDOWN:
      nextFrameState->input.mouse.left.pressedDown = true;
      nextFrameState->input.mouse.left.isDown = true;
      break;
    case WM_LBUTTONUP:
      nextFrameState->input.mouse.left.pressedUp = true;
      nextFrameState->input.mouse.left.isDown = false;
      break;
    case WM_MBUTTONDOWN:
      nextFrameState->input.mouse.middle.pressedDown = true;
      nextFrameState->input.mouse.middle.isDown = true;
      break;
    case WM_MBUTTONUP:
      nextFrameState->input.mouse.middle.pressedUp = true;
      nextFrameState->input.mouse.middle.isDown = false;
      break;
    case WM_RBUTTONDOWN:
      nextFrameState->input.mouse.right.pressedDown = true;
      nextFrameState->input.mouse.right.isDown = true;
      break;
    case WM_RBUTTONUP:
      nextFrameState->input.mouse.right.pressedUp = true;
      nextFrameState->input.mouse.right.isDown = false;
      break;
    case WM_MOUSEWHEEL:
      nextFrameState->input.mouse.dWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
      break;
    case WM_KEYDOWN:
      switch (wParam) {
      case VK_ESCAPE:
        PostQuitMessage(0);
        break;
      case VK_F1:
        nextFrameState->input.keyboard.F1.pressedDown = true;
        break;
      case VK_MENU:
        nextFrameState->input.keyboard.rightAlt.pressedDown = true;
        break;
      case VK_RETURN:
        nextFrameState->input.keyboard.enter.pressedDown = true;
        break;
      case VK_LEFT:
        nextFrameState->input.keyboard.left.pressedDown = true;
        break;
      case VK_UP:
        nextFrameState->input.keyboard.up.pressedDown = true;
        break;
      case VK_RIGHT:
        nextFrameState->input.keyboard.right.pressedDown = true;
        break;
      case VK_DOWN:
        nextFrameState->input.keyboard.down.pressedDown = true;
        break;
      case VK_SPACE:
        nextFrameState->input.keyboard.space.pressedDown = true;
        break;
      case VK_Q:
        nextFrameState->input.keyboard.q.pressedDown = true;
        break;
      case VK_W:
        nextFrameState->input.keyboard.w.pressedDown = true;
        break;
      case VK_E:
        nextFrameState->input.keyboard.e.pressedDown = true;
        break;
      case VK_A:
        nextFrameState->input.keyboard.a.pressedDown = true;
        break;
      case VK_S:
        nextFrameState->input.keyboard.s.pressedDown = true;
        break;
      case VK_D:
        nextFrameState->input.keyboard.d.pressedDown = true;
        break;
      }
      break;
    case WM_KEYUP:
      switch (wParam) {
      case VK_F1:
        nextFrameState->input.keyboard.F1.pressedUp = true;
        break;
      case VK_MENU:
        nextFrameState->input.keyboard.rightAlt.pressedUp = true;
        break;
      case VK_RETURN:
        nextFrameState->input.keyboard.enter.pressedUp = true;
        break;
      case VK_LEFT:
        nextFrameState->input.keyboard.left.pressedUp = true;
        break;
      case VK_UP:
        nextFrameState->input.keyboard.up.pressedUp = true;
        break;
      case VK_RIGHT:
        nextFrameState->input.keyboard.right.pressedUp = true;
        break;
      case VK_DOWN:
        nextFrameState->input.keyboard.down.pressedUp = true;
        break;
      case VK_SPACE:
        nextFrameState->input.keyboard.space.pressedUp = true;
        break;
      case VK_Q:
        nextFrameState->input.keyboard.q.pressedUp = true;
        break;
      case VK_W:
        nextFrameState->input.keyboard.w.pressedUp = true;
        break;
      case VK_E:
        nextFrameState->input.keyboard.e.pressedUp = true;
        break;
      case VK_A:
        nextFrameState->input.keyboard.a.pressedUp = true;
        break;
      case VK_S:
        nextFrameState->input.keyboard.s.pressedUp = true;
        break;
      case VK_D:
        nextFrameState->input.keyboard.d.pressedUp = true;
        break;
      }
    default:
      result = DefWindowProc(windowHandle, message, wParam, lParam);
      break;
    }
    return result;
  }
}

static void debugShowResourcesUsage()
{
#ifdef DAR_DEBUG
  // from https://stackoverflow.com/a/64166/9178254

  // CPU
  static ULARGE_INTEGER lastCPU = {}, lastSysCPU = {}, lastUserCPU = {};
  static double percent = 0.;
  static DWORD lastCheckTimeMs = 0;
  DWORD currentTimeMs = GetTickCount();
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

static void showErrorMessageBox(const char* text, const char* caption)
{
  MessageBoxA(window, text, caption, MB_OK | MB_ICONERROR);
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
    showErrorMessageBox("Failed to register window class.", "Fatal error");
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
    showErrorMessageBox("Failed to create game window", "Fatal error");
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

  const int workerThreadCount = max(coreCount - 1, 1);
  taskScheduler.initialize(workerThreadCount, 1);

  D3D11Renderer renderer(window);

  //Audio audio;

  //Game game;

  ShowWindow(window, SW_SHOWNORMAL);

  Vec2i cursorPosition = getCursorPosition();
  lastFrameState = frameStates.getLastState(frameCount);
  lastFrameState->input.cursorPosition = cursorPosition;
  nextFrameState = frameStates.getNextState(frameCount);

  LARGE_INTEGER counterFrequency;
  QueryPerformanceFrequency(&counterFrequency);
  LARGE_INTEGER lastCounterValue;
  QueryPerformanceCounter(&lastCounterValue);

  MSG message{};
  while (message.message != WM_QUIT) {
    if (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageA(&message);
    }
    else {
      // process frame

      nextFrameState->input.cursorPosition = getCursorPosition();
      nextFrameState->clientAreaWidth = clientAreaWidth;
      nextFrameState->clientAreaHeight = clientAreaHeight;

      LARGE_INTEGER currentCounterValue;
      QueryPerformanceCounter(&currentCounterValue);
      nextFrameState->deltaTime = (float)(currentCounterValue.QuadPart - lastCounterValue.QuadPart) / counterFrequency.QuadPart;
      lastCounterValue = currentCounterValue;

      debugResetText();
      debugText(L"%.3f s / %d fps", nextFrameState->deltaTime, (int)(1.f / nextFrameState->deltaTime));
      debugShowResourcesUsage();

      //game.update(*lastFrameState, nextFrameState);

      if (lastFrameState->clientAreaWidth != clientAreaWidth || lastFrameState->clientAreaHeight != clientAreaHeight) {
          renderer.onWindowResize(clientAreaWidth, clientAreaHeight);
      }

      renderer.render(*nextFrameState);

      //audio.update(*nextFrameState);

      ++frameCount;

      lastFrameState = frameStates.getLastState(frameCount);
      nextFrameState = frameStates.getNextState(frameCount);
      nextFrameState->input = lastFrameState->input;
      nextFrameState->input.keyboard = {};
      nextFrameState->input.mouse.dWheel = 0.f;
    }
  }
  return 0;
}