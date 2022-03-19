#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"
#include "Core/Task.hpp"

class Heightmap
{
public:

  Heightmap() = default;
  Heightmap(const Heightmap& other) = delete;
  Heightmap(const Heightmap&& other) = delete;
  ~Heightmap();

  int16* data = nullptr; // elevation in meters.
  int32 width = 0;
  int32 height = 0;

  bool tryLoad(const wchar_t* mapName);

  int64 getDataSize() const { return width * height * 2; }

private:

  void reset();

};

// Holds data with lifetime of a map
struct Map
{
  Heightmap heightmap;
  int64 widthInMeters = 30;
  int64 heightInMeters = 40;

  float cameraZoomMin;
  float cameraZoomMax;

  bool tryLoad(const wchar_t* mapName, float verticalFieldOfViewRadians, float aspectRatio);
};