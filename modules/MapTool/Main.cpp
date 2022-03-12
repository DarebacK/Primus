#include "Core/Core.hpp"
#include "Core/Image.hpp"

#include <WinInet.h>

#include <vector>
#include <memory>
#include <fstream>

const wchar_t* server = L"tile.nextzen.org";
const wchar_t* requestObjectName = L"tilezen/terrain/v1/512/terrarium/8/139/98.png?api_key=XeM2Tf-mRS6G1orxmShMzg";

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

int main(int argc, char* argv[])
{
  InternetHandle internet = InternetOpen(L"Primus MapTool", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
  if (!internet)
  {
    logError("InternetOpen failed.");
    return 1;
  }

  InternetHandle connect = InternetConnect(internet, server, INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, NULL);
  if (!connect)
  {
    logError("InternetConnect failed.");
    return 2;
  }

  const wchar_t* acceptTypes[] = { L"image/png", NULL };
  constexpr DWORD httpRequestFlags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_SECURE;
  InternetHandle httpRequest = HttpOpenRequest(connect, L"GET", requestObjectName, nullptr, nullptr, acceptTypes, httpRequestFlags, NULL);
  if(!httpRequest)
  {
    logError("HttpOpenRequest failed.");
    return 3;
  }

  if (!HttpSendRequest(httpRequest, nullptr, 0, nullptr, 0))
  {
    logError("HttpSendRequest failed.");
    return 4;
  }

  DWORD statusCode = queryRequestNumber(httpRequest, HTTP_QUERY_STATUS_CODE);
  if (statusCode != 200)
  {
    logError("Http request failed. Status code %lu.", statusCode);
    return 5;
  }

  DWORD contentLength = queryRequestNumber(httpRequest, HTTP_QUERY_CONTENT_LENGTH);
  if (contentLength == 0)
  {
    logError("Failed to get http request content length");
    return 6;
  }

  std::vector<byte> contentBuffer;
  contentBuffer.resize(contentLength);
  uint64 contentBufferWriteOffset = 0;
  while (true)
  {
    DWORD bytesRead;
    if (!InternetReadFile(httpRequest, contentBuffer.data() + contentBufferWriteOffset, contentLength, &bytesRead))
    {
      if (contentBufferWriteOffset != contentLength)
      {
        logError("InternetReadFile failed.");
        return 7;
      }
      else
      {
        break;
      }
    }

    if (bytesRead == 0)
    {
      break;
    }

    contentBufferWriteOffset += bytesRead;
  }

  PngReadResult pngResult = readPng(contentBuffer.data(), contentLength);
  if (!pngResult.data)
  {
    logError("Failed to read PNG data.");
    return 8;
  }

  if (pngResult.channelCount != 4)
  {
    logError("PNG data has wrong channel count %lld", pngResult.channelCount);
    return 9;
  }

  const byte* readDataIterator = pngResult.data;
  uint16* writeDataIterator = reinterpret_cast<uint16*>(pngResult.data);
  for (int64 y = 0; y < pngResult.height; ++y)
  {
    for (int64 x = 0; x < pngResult.width; ++x)
    {
      uint8 r = *readDataIterator++;
      uint8 g = *readDataIterator++;
      uint8 b = *readDataIterator++;
      uint16 elevationInMeters = static_cast<uint16>(r) * 256 + static_cast<uint16>(g) + round(float(b) / 256) - static_cast<uint16>(32768);
      *writeDataIterator++ = elevationInMeters;
      readDataIterator++; // skip alpha, it has no information anyway.
    }
  }


  if (!writePng("output.png", pngResult.data, pngResult.width, pngResult.height, 1, pngResult.width * 2))
  {
    logError("Failed to write png data.");
    return 10;
  }

  return 0;
}