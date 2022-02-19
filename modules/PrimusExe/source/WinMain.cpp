#define DAR_MODULE_NAME "WinMain"

#include <exception>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <Psapi.h>

#include "Core/Core.hpp"
#include "Math/Math.hpp"

#define VK_Q 0x51
#define VK_W 0x57
#define VK_E 0x45
#define VK_A 0x41
#define VK_S 0x53
#define VK_D 0x44

namespace
{
  class GameStates
  {
  public:
    GameState* getLastState(unsigned int frameIndex) { return states + (--frameIndex % size); };  // overflow when frameIndex == 0 shouldn't be a problem
    GameState* getNextState(unsigned int frameIndex) { return states + (frameIndex % size); };

  private:
    static constexpr int size = 2;
    GameState states[size] = {};
  };

  int clientAreaWidth = GetSystemMetrics(SM_CXSCREEN);
  int clientAreaHeight = GetSystemMetrics(SM_CYSCREEN);
  HWND window = nullptr;
  HANDLE process = nullptr;
  WINDOWPLACEMENT windowPosition = { sizeof(windowPosition) };
  const char* gameName = "Demo";
  int processorCount = 0;
  GameStates gameStates;
  GameState* lastGameState = nullptr;
  UserInput* nextUserInput = nullptr;
  int frameCount = 0;
  D3D11Renderer* rendererPtr = nullptr;

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
      if (newClientAreaWidth != clientAreaWidth || newClientAreaHeight != clientAreaHeight) {
        clientAreaWidth = newClientAreaWidth;
        clientAreaHeight = newClientAreaHeight;
        if (rendererPtr) {
          rendererPtr->onWindowResize(clientAreaWidth, clientAreaHeight);
        }
      }
    }
                break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_LBUTTONDOWN:
      nextUserInput->mouse.left.pressedDown = true;
      nextUserInput->mouse.left.isDown = true;
      break;
    case WM_LBUTTONUP:
      nextUserInput->mouse.left.pressedUp = true;
      nextUserInput->mouse.left.isDown = false;
      break;
    case WM_MBUTTONDOWN:
      nextUserInput->mouse.middle.pressedDown = true;
      nextUserInput->mouse.middle.isDown = true;
      break;
    case WM_MBUTTONUP:
      nextUserInput->mouse.middle.pressedUp = true;
      nextUserInput->mouse.middle.isDown = false;
      break;
    case WM_RBUTTONDOWN:
      nextUserInput->mouse.right.pressedDown = true;
      nextUserInput->mouse.right.isDown = true;
      break;
    case WM_RBUTTONUP:
      nextUserInput->mouse.right.pressedUp = true;
      nextUserInput->mouse.right.isDown = false;
      break;
    case WM_MOUSEWHEEL:
      nextUserInput->mouse.dWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
      break;
    case WM_KEYDOWN:
      switch (wParam) {
      case VK_ESCAPE:
        PostQuitMessage(0);
        break;
      case VK_F1:
        nextUserInput->keyboard.F1.pressedDown = true;
        break;
      case VK_MENU:
        nextUserInput->keyboard.rightAlt.pressedDown = true;
        break;
      case VK_RETURN:
        nextUserInput->keyboard.enter.pressedDown = true;
        break;
      case VK_LEFT:
        nextUserInput->keyboard.left.pressedDown = true;
        break;
      case VK_UP:
        nextUserInput->keyboard.up.pressedDown = true;
        break;
      case VK_RIGHT:
        nextUserInput->keyboard.right.pressedDown = true;
        break;
      case VK_DOWN:
        nextUserInput->keyboard.down.pressedDown = true;
        break;
      case VK_SPACE:
        nextUserInput->keyboard.space.pressedDown = true;
        break;
      case VK_Q:
        nextUserInput->keyboard.q.pressedDown = true;
        break;
      case VK_W:
        nextUserInput->keyboard.w.pressedDown = true;
        break;
      case VK_E:
        nextUserInput->keyboard.e.pressedDown = true;
        break;
      case VK_A:
        nextUserInput->keyboard.a.pressedDown = true;
        break;
      case VK_S:
        nextUserInput->keyboard.s.pressedDown = true;
        break;
      case VK_D:
        nextUserInput->keyboard.d.pressedDown = true;
        break;
      }
      break;
    case WM_KEYUP:
      switch (wParam) {
      case VK_F1:
        nextUserInput->keyboard.F1.pressedUp = true;
        break;
      case VK_MENU:
        nextUserInput->keyboard.rightAlt.pressedUp = true;
        break;
      case VK_RETURN:
        nextUserInput->keyboard.enter.pressedUp = true;
        break;
      case VK_LEFT:
        nextUserInput->keyboard.left.pressedUp = true;
        break;
      case VK_UP:
        nextUserInput->keyboard.up.pressedUp = true;
        break;
      case VK_RIGHT:
        nextUserInput->keyboard.right.pressedUp = true;
        break;
      case VK_DOWN:
        nextUserInput->keyboard.down.pressedUp = true;
        break;
      case VK_SPACE:
        nextUserInput->keyboard.space.pressedUp = true;
        break;
      case VK_Q:
        nextUserInput->keyboard.q.pressedUp = true;
        break;
      case VK_W:
        nextUserInput->keyboard.w.pressedUp = true;
        break;
      case VK_E:
        nextUserInput->keyboard.e.pressedUp = true;
        break;
      case VK_A:
        nextUserInput->keyboard.a.pressedUp = true;
        break;
      case VK_S:
        nextUserInput->keyboard.s.pressedUp = true;
        break;
      case VK_D:
        nextUserInput->keyboard.d.pressedUp = true;
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
    percent /= double(processorCount);
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
try
{
  process = GetCurrentProcess();

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  processorCount = sysInfo.dwNumberOfProcessors;

  WNDCLASS windowClass{};
  windowClass.lpfnWndProc = &WindowProc;
  windowClass.hInstance = instanceHandle;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = "Game window class";
  if (!RegisterClassA(&windowClass)) {
    showErrorMessageBox("Failed to register window class.", "Fatal error");
    return -1;
  }

  RECT windowRectangle = { 0, 0, clientAreaWidth, clientAreaHeight };
  constexpr DWORD windowStyle = WS_POPUP;
  constexpr DWORD windowStyleEx = 0;
  AdjustWindowRectEx(&windowRectangle, windowStyle, false, windowStyleEx);
  const int windowWidth = windowRectangle.right - windowRectangle.left;
  const int windowHeight = windowRectangle.bottom - windowRectangle.top;
  window = CreateWindowExA(
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

  VulkanRenderer renderer(window);
  rendererPtr = &renderer;

  Audio audio;

  Game game;

  ShowWindow(window, SW_SHOWNORMAL);

  Vec2i cursorPosition = getCursorPosition();
  lastGameState = gameStates.getLastState(frameCount);
  lastGameState->input.cursorPosition = cursorPosition;
  lastGameState->events.emplace(Event::GameStarted, Event());
  lastGameState->phase = GameState::Phase::Playing;
  nextGameState = gameStates.getNextState(frameCount);

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

      nextUserInput->cursorPosition = getCursorPosition();
      nextGameState->clientAreaWidth = clientAreaWidth;
      nextGameState->clientAreaHeight = clientAreaHeight;

      LARGE_INTEGER currentCounterValue;
      QueryPerformanceCounter(&currentCounterValue);
      nextGameState->dTime = (float)(currentCounterValue.QuadPart - lastCounterValue.QuadPart) / counterFrequency.QuadPart;
      lastCounterValue = currentCounterValue;

      debugResetText();
      debugText(L"%.3f s / %d fps", nextGameState->dTime, (int)(1.f / nextGameState->dTime));
      debugShowResourcesUsage();

      game.update(*lastGameState, nextGameState);

      renderer.render(*nextGameState);

      audio.update(*nextGameState);

      ++frameCount;

      lastGameState = gameStates.getLastState(frameCount);
      nextGameState = gameStates.getNextState(frameCount);
      nextGameState->input = lastGameState->input;
      nextUserInput->keyboard = {};
      nextUserInput->mouse.dWheel = 0.f;
      nextGameState->events.clear();
    }
  }
  return 0;
}
catch (const std::exception& e) {
  showErrorMessageBox(e.what(), "Fatal error");
  return -2;
}
catch (...) {
  showErrorMessageBox("Unknown error", "Fatal error");
  return -3;
}
