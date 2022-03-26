#pragma once

#include "Core/Core.hpp"

#include <WinInet.h>

#include <vector>
#include <memory>

struct InternetHandleDeleter {
  void operator()(HINTERNET handle) { InternetCloseHandle(handle); }
};
using InternetHandle = std::unique_ptr<void, InternetHandleDeleter>;

class HttpDownloader
{
public:

  HttpDownloader(HINTERNET internet, const wchar_t* server);

  bool tryDownloadImage(const wchar_t* url, std::vector<byte>& buffer);

private:
  
  InternetHandle serverConnection;
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
