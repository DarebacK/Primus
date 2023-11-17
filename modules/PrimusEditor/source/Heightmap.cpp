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

void writeHeightmapToObj(const Ref<Texture2D>& heightmap)
{
  // TODO: Separate heightmap into tiles of 256x256, which will be frustum culled independently and
  //       do limited dissolve in Blender to simplify the mesh as there is a lot of coplanar triangles.
  //       Try to separate land and water geometry in Blender and put it in a separate tile file (with the same coordinates though)

  {
    TRACE_SCOPE("CreateTerrainObjFile");

    heightmap->initializedTaskEvent->waitForCompletion();
    std::ofstream obj{ "terrain.obj" };
    if(ensure(obj.is_open()))
    {
      for(int32 y = 0; y < heightmap->height; ++y)
      {
        const float z = 1.f - (y / float(heightmap->height - 1));

        for(int32 x = 0; x < heightmap->width; ++x)
        {
          int16 heightInMeters = heightmap->sample<int16>(x, y);
          heightInMeters = std::max(heightInMeters, 0i16);

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
}