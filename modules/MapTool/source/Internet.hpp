#pragma once

#include "Core/Core.hpp"

#include <WinInet.h>

class InternetHandle
{
public:
  InternetHandle(HINTERNET inHandle) : handle(inHandle) {}
  InternetHandle(InternetHandle& other) = delete;
  InternetHandle(InternetHandle&& other) = delete;
  ~InternetHandle() { InternetCloseHandle(handle); }

  operator HINTERNET() { return handle; }

private:
  HINTERNET handle;
};

static DWORD queryRequestNumber(HINTERNET request, DWORD infoLevel)
{
  byte queryBuffer[32] = {};
  DWORD queryBufferLength = static_cast<DWORD>(sizeof(queryBuffer));
  DWORD headerIndex = 0;
  if (!HttpQueryInfo(request, infoLevel | HTTP_QUERY_FLAG_NUMBER, queryBuffer, &queryBufferLength, &headerIndex))
  {
    logError("HttpQueryInfo failed.");
    return 0;
  }

  return *reinterpret_cast<DWORD*>(queryBuffer);
}
