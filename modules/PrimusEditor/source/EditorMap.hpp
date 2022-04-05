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
  Colormap colormap;

  bool tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);
};