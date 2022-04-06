#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"

#include "Primus/Map.hpp"

struct Colormap
{
  Image image;
};

// Holds data related to map to be used in the editor.
struct EditorMap : public Map
{
  int16 heightmapTileXMin = 66;
  int16 heightmapTileXMax = 70;
  int16 heightmapTileYMin = 45;
  int16 heightmapTileYMax = 50;
  int16 heightmapTileZoom = 7;
  int16 heightmapTileSize = 512;

  Colormap colormap;

  bool tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);

  bool tryFixLandElevation();
};