#include "Heightmap.hpp"

#include "Core/Image.hpp"
#include "Core/File.hpp"

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
}

void downloadHeightmap(HINTERNET internet, const char* outputFileName)
{
  HttpDownloader downloader{ internet, serverUrl };

  std::vector<uint16> heightmap;
  heightmap.resize(tileCount * TILE_SIZE * TILE_SIZE);

  std::vector<byte> contentBuffer;
  uint16 minElevation = std::numeric_limits<uint16>::max();
  uint16 maxElevation = 0;
  for (int32 tileY = tileYMin; tileY <= tileYMax; tileY++)
  {
    const int64 tileYIndex = tileY - tileYMin;

    for (int32 tileX = tileXMin; tileX <= tileXMax; tileX++)
    {
      const int64 tileXIndex = tileX - tileXMin;

      wchar_t url[128];
      swprintf_s(url, L"%ls/%d/%d%ls", requestUrlBegin, tileX, tileY, requestUrlEnd);

      printf("Downloading %S\n", url);

      downloader.tryDownloadImage(url, contentBuffer);

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
          uint16 elevationInMeters = uint16(r) * uint16(256) + uint16(g) + uint16(round(float(b) / 256)) - uint16(32768);
          int64 heightmapIndex = tileYIndex * widthInPixels * TILE_SIZE + tileXIndex * TILE_SIZE + y * widthInPixels + x;

          minElevation = std::min(minElevation, elevationInMeters);
          maxElevation = std::max(maxElevation, elevationInMeters);
          heightmap[heightmapIndex] = elevationInMeters;

          readDataIterator++; // skip alpha, it has no information anyway.
        }
      }
    }
  }

  wchar_t outputFilePath[MAX_PATH];
  swprintf_s(outputFilePath, L"%S.s16", outputFileName);
  if (!tryWriteFile(outputFilePath, (const byte*)heightmap.data(), heightmap.size() * sizeof(int16)))
  {
    logError("Failed to write heightmap data to a file.");
    return;
  }
}

void exportHeightmapToObj(const EditorMap& map, const wchar_t* path)
{
  TRACE_SCOPE();

  Texture2D* heightmap = map.heightmap.get();

  heightmap->initializedTaskEvent->waitForCompletion();
  std::ofstream obj{ path };
  if(ensure(obj.is_open()))
  {
    for(int32 y = 0; y < heightmap->height; ++y)
    {
      const float z = 1.f - (y / float(heightmap->height - 1));

      for(int32 x = 0; x < heightmap->width; ++x)
      {
        int16 heightInMeters = heightmap->sample<int16>(x, y);
        heightInMeters = std::max(heightInMeters, 0i16);

        // TODO: export in "realistic" units, i.e. x y z are in the same units, so the mesh simplifier in Blender works correctly
        obj << "v " << x / float(heightmap->width - 1) << ' ' << heightInMeters / 1000.f << ' ' << z << '\n';
      }
    }

    for(int32 y = 0; y < heightmap->height; ++y)
    {
      // Flip it for blender
      const float v = 1.f - (y / float(heightmap->height - 1));

      for(int32 x = 0; x < heightmap->width; ++x)
      {
        obj << "vt " << x / float(heightmap->width - 1) << ' ' << v << '\n';
      }
    }

    for(int32 y = 0; y < heightmap->height - 1; ++y)
    {
      for(int32 x = 1; x < heightmap->width - 1; ++x)
      {
        // TODO: this doesn't seem correct due to vertices starting at z = 1. In tiled export it is vice versa.
        // v3   v4
        // _______
        // |\    |
        // |  X  |
        // | ___\|
        // v1   v2
        const int64 v1 = y * heightmap->width + x;
        const int64 v2 = v1 + 1;
        const int64 v3 = (y + 1) * heightmap->width + x;
        const int64 v4 = v3 + 1;

        obj << "f " << v3 << '/' << v3 << ' ' << v1 << '/' << v1 << ' ' << v2 << '/' << v2 << '\n';
        obj << "f " << v3 << '/' << v3 << ' ' << v2 << '/' << v2 << ' ' << v4 << '/' << v4 << '\n';
      }
    }
  }
}

void exportHeightmapToObjTiles(const EditorMap& map, const wchar_t* path)
{
  TRACE_SCOPE();

  // TODO: add skirts to the sides to hide the pixel hole between tiles

  Texture2D* heightmap = map.heightmap.get();

  constexpr int32 tileSize = 256;
  const int32 tileCountX = heightmap->width / tileSize;
  ensure(heightmap->width % tileSize == 0);
  const int32 tileCountY = heightmap->height / tileSize;
  ensure(heightmap->width % tileSize == 0);

  const float xMapCoefficient = (map.widthInM / 1000.f) / float(heightmap->width - 1);
  const float yMapCoefficient = map.visualHeightMultiplier;
  const float zMapCoefficient = map.heightInM / 1000.f;

  heightmap->initializedTaskEvent->waitForCompletion();

  for(int32 tileY = 0; tileY < tileCountY; tileY++)
  {
    const int32 yMin = tileY * tileSize;
    const int32 yMax = (tileY + 1) * tileSize;

    for(int32 tileX = 0; tileX < tileCountX; tileX++)
    {
      wchar_t tilePath[MAX_PATH] = L"";
      wsprintf(tilePath, L"%s\\combinedTile_Y%d_X%d.obj", path, tileY, tileX);
      std::ofstream obj{ tilePath };
      if(!ensure(obj.is_open()))
      {
        continue;
      }

      const int32 xMin = tileX * tileSize;
      const int32 xMax = (tileX + 1) * tileSize;

      for(int32 y = yMin; y <= std::min(yMax, heightmap->height - 1) ; ++y)
      {
        const float z = (1.f - (y / float(heightmap->height - 1))) * zMapCoefficient;

        for(int32 x = xMin; x <= std::min(xMax, heightmap->width - 1); ++x)
        {
          int16 heightInMeters = heightmap->sample<int16>(x, y);
          heightInMeters = std::max(heightInMeters, 0i16);

          obj << "v " << x * xMapCoefficient  << ' ' << heightInMeters * yMapCoefficient << ' ' << z << '\n';
        }
      }

      for(int32 y = yMin; y <= std::min(yMax, heightmap->height - 1); ++y)
      {
        // Flip it for blender
        const float v = 1.f - (y / float(heightmap->height - 1));

        for(int32 x = xMin; x <= std::min(xMax, heightmap->width - 1); ++x)
        {
          obj << "vt " << x / float(heightmap->width - 1) << ' ' << v << '\n';
        }
      }

      for(int32 y = 0; y < tileSize; ++y)
      {
        for(int32 x = 1; x <= tileSize; ++x)
        {
          // v1   v2
          // _______
          // |\    |
          // |  X  |
          // | ___\|
          // v3   v4
          const int64 v1 = y * (tileSize + 1) + x;
          const int64 v2 = v1 + 1;
          const int64 v3 = (y + 1) * (tileSize + 1) + x;
          const int64 v4 = v3 + 1;

          obj << "f " << v4 << '/' << v4 << ' ' << v1 << '/' << v1 << ' ' << v3 << '/' << v3 << '\n';
          obj << "f " << v4 << '/' << v4 << ' ' << v2 << '/' << v2 << ' ' << v1 << '/' << v1 << '\n';
        }
      }
    }
  }
}