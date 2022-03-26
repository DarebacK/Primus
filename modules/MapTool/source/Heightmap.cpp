#include "Heightmap.hpp"

#include "Core/Image.hpp"

#include <vector>
#include <memory>
#include <fstream>

#define TILE_SIZE 512
#define TILE_ZOOM 7

namespace {
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

  const wchar_t* serverUrl = L"tile.nextzen.org";
  const wchar_t* requestUrlBegin = L"tilezen/terrain/v1/" WSTRINGIFY_DEFINE(TILE_SIZE) L"/terrarium/" WSTRINGIFY_DEFINE(TILE_ZOOM);
  const wchar_t* requestUrlEnd = L".png?api_key=XeM2Tf-mRS6G1orxmShMzg";

  constexpr uint16 elevationOffset = 11000; // Used to convert below sea elevations to unsigned int range. 11000 because deepest point is ~10935 meters.
}

void downloadHeightmap(HINTERNET internet, const char* outputFilePath)
{
  HttpDownloader downloader{ internet, serverUrl };

  std::vector<uint16> heightmap;
  heightmap.resize(tileCount * TILE_SIZE * TILE_SIZE * 2);

  uint16 minElevation = std::numeric_limits<uint16>::max();
  uint16 maxElevation = 0;
  for (int32 tileY = tileYMin; tileY <= tileYMax; tileY++)
  {
    const int64 tileYIndex = tileY - tileYMin;

    for (int32 tileX = tileXMin; tileX <= tileXMax; tileX++)
    {
      const int64 tileXIndex = tileX - tileXMin;

      wchar_t requestObjectName[128];
      swprintf_s(requestObjectName, L"%ls/%d/%d%ls", requestUrlBegin, tileX, tileY, requestUrlEnd);

      printf("Downloading %S\n", requestObjectName);

      std::vector<byte> contentBuffer;
      downloader.tryDownloadImage(requestObjectName, contentBuffer);

      PngReadResult pngResult = readPng(contentBuffer.data(), int64(contentBuffer.size()));
      if (!pngResult.data)
      {
        logError("Failed to read PNG data.");
        return;
      }

      if (pngResult.channelCount != 4) // For some reason the terarrium image file has alpha channel despite it not containing any information.
      {
        logError("PNG data has wrong channel count %lld", pngResult.channelCount);
        return;
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
          int64 heightmapIndex = tileYIndex * widthInPixels * TILE_SIZE + tileXIndex * TILE_SIZE + y * widthInPixels + x;
          if constexpr (useAutoExposure)
          {
            minElevation = std::min(minElevation, elevationInMeters);
            maxElevation = std::max(maxElevation, elevationInMeters);
            heightmap[heightmapIndex] = elevationInMeters;
          }
          else
          {
            heightmap[heightmapIndex] = nativeToBigEndian(elevationInMeters);
          }
          readDataIterator++; // skip alpha, it has no information anyway.
        }
      }
    }
  }

  if constexpr (useAutoExposure)
  {
    const float multiplier = std::numeric_limits<uint16>::max() / float(maxElevation - minElevation);
    for (uint16& pixel : heightmap)
    {
      pixel = nativeToBigEndian(uint16((pixel - minElevation) * multiplier));
    }
  }

  if (!writePngGrayscaleBigEndian(outputFilePath, reinterpret_cast<byte*>(heightmap.data()), widthInPixels, heightInPixels, 1, 16))
  {
    logError("Failed to write png data.");
    return;
  }
}