#define NOMINMAX

#include "Core/Core.hpp"
#include "Core/Image.hpp"

#include <WinInet.h>

#include <vector>
#include <memory>
#include <fstream>

#define TILE_SIZE 512
#define TILE_ZOOM 7

constexpr int64 tileXMin = 66;
constexpr int64 tileXMax = 70; 
constexpr int64 widthInTiles = tileXMax - tileXMin + 1;
constexpr int64 widthInPixels = widthInTiles * TILE_SIZE;
constexpr int64 tileYMin = 45;
constexpr int64 tileYMax = 50;
constexpr int64 heightInTiles = tileYMax - tileYMin + 1;
constexpr int64 heightInPixels = TILE_SIZE * heightInTiles;
constexpr int64 tileCount = widthInTiles * heightInTiles;
constexpr bool useAutoExposure = false;

const wchar_t* server = L"tile.nextzen.org";
const wchar_t* requestObjectNameBegin = L"tilezen/terrain/v1/" WSTRINGIFY_DEFINE(TILE_SIZE) L"/terrarium/" WSTRINGIFY_DEFINE(TILE_ZOOM);
const wchar_t* requestObjectNameEnd = L".png?api_key=XeM2Tf-mRS6G1orxmShMzg";

constexpr uint16 elevationOffset = 11000; // Used to convert below sea elevations to unsigned int range. 11000 because deepest point is ~10935 meters.

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

  std::vector<uint16> heightmap;
  heightmap.resize(tileCount * TILE_SIZE * TILE_SIZE * 2);

  const wchar_t* acceptTypes[] = { L"image/png", NULL };
  constexpr DWORD httpRequestFlags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_SECURE;

  uint16 minElevation = std::numeric_limits<uint16>::max();
  uint16 maxElevation = 0;
  for (int32 tileY = tileYMin; tileY <= tileYMax; tileY++)
  {
    const int64 tileYIndex = tileY - tileYMin;

    for (int32 tileX = tileXMin; tileX <= tileXMax; tileX++)
    {
      const int64 tileXIndex = tileX - tileXMin;

      wchar_t requestObjectName[128];
      swprintf_s(requestObjectName, L"%ls/%d/%d%ls", requestObjectNameBegin, tileX, tileY, requestObjectNameEnd);

      InternetHandle httpRequest = HttpOpenRequest(connect, L"GET", requestObjectName, nullptr, nullptr, acceptTypes, httpRequestFlags, NULL);
      if (!httpRequest)
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

      if (pngResult.channelCount != 4) // For some reason the terarrium image file has alpha channel despite it not containing any information.
      {
        logError("PNG data has wrong channel count %lld", pngResult.channelCount);
        return 9;
      }

      const byte* readDataIterator = pngResult.data;
      for (int64 y = 0; y < pngResult.height; ++y)
      {
        for (int64 x = 0; x < pngResult.width; ++x)
        {
          uint8 r = *readDataIterator++;
          uint8 g = *readDataIterator++;
          uint8 b = *readDataIterator++;
          uint16 elevationInMeters = uint16(r) * uint16(256) + uint16(g) + uint16(round(float(b) / 256)) - uint16(32768 - elevationOffset); // elevation offset prevents going into negative numbers.
          if constexpr (useAutoExposure)
          {
            minElevation = std::min(minElevation, elevationInMeters);
            maxElevation = std::max(maxElevation, elevationInMeters);
          }
          heightmap[tileYIndex*widthInPixels*TILE_SIZE + tileXIndex*TILE_SIZE + y*widthInPixels + x] = elevationInMeters;
          readDataIterator++; // skip alpha, it has no information anyway.
        }
      }
    }
  }

  const float multiplier = std::numeric_limits<uint16>::max() / float(maxElevation - minElevation);
  for (uint16& pixel : heightmap)
  {
    if constexpr (useAutoExposure)
    {
      pixel = (pixel - minElevation) * multiplier;
    }
    pixel = nativeToBigEndian(pixel);
  }

  if (!writePngLosslessGrayscaleBigEndian("heightmap.png", reinterpret_cast<byte*>(heightmap.data()), widthInPixels, heightInPixels, 1, 16))
  {
    logError("Failed to write png data.");
    return 10;
  }

  return 0;
}