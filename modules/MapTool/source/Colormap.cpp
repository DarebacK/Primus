#include "Colormap.hpp"

#include "Core/Image.hpp"

#include <vector>
#include <memory>
#include <fstream>

namespace {
  constexpr int tileSize = 512;
  #define TILE_ZOOM 10

  constexpr int64 tileXMin = 547;
  constexpr int64 tileXMax = 547;
  constexpr int64 widthInTiles = tileXMax - tileXMin + 1;
  constexpr int64 widthInPixels = widthInTiles * tileSize;
  constexpr int64 tileYMin = 380;
  constexpr int64 tileYMax = 380;
  constexpr int64 heightInTiles = tileYMax - tileYMin + 1;
  constexpr int64 heightInPixels = tileSize * heightInTiles;
  constexpr int64 tileCount = widthInTiles * heightInTiles;

  // https://api.maptiler.com/tiles/satellite-mediumres/{z}/{x}/{y}.jpg?key=jv0AKvuMCw6M6sWGYF8M
  const wchar_t* serverUrl = L"api.maptiler.com";
  const wchar_t* requestUrlBegin = L"tiles/satellite-mediumres/" WSTRINGIFY_DEFINE(TILE_ZOOM) L"/";
  const wchar_t* requestUrlEnd = L".jpg?key=jv0AKvuMCw6M6sWGYF8M";
}

void downloadColormap(HINTERNET internet, const char* outputFilePath)
{
  HttpDownloader downloader{ internet, serverUrl };

  std::vector<uint16> colormap;
  colormap.resize(tileCount * tileSize * tileSize * 2);

  std::vector<byte> contentBuffer;
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
    }
  }

  FILE* file;
  if (fopen_s(&file, "colormap.jpg", "wb") == 0)
  {
    fwrite(contentBuffer.data(), contentBuffer.size(), 1, file);
    fclose(file);
  }
}