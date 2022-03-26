#include "Colormap.hpp"

#include "Core/Image.hpp"

#include <vector>
#include <memory>
#include <fstream>

namespace {
  constexpr int tileSize = 512;
  #define TILE_ZOOM 8

  constexpr int64 tileXMin = 66;
  constexpr int64 tileXMax = 70;
  constexpr int64 widthInTiles = tileXMax - tileXMin + 1;
  constexpr int64 widthInPixels = widthInTiles * tileSize;
  constexpr int64 tileYMin = 45;
  constexpr int64 tileYMax = 50;
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

}