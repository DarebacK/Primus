#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"
#include "Core/Task.hpp"

class Heightmap
{
public:

  Heightmap() = default;
  Heightmap(const Heightmap& other) = delete;
  Heightmap(Heightmap&& other) noexcept;
  ~Heightmap();

  int32 width = 0;
  int32 height = 0;

  bool tryLoad(const wchar_t* mapName);

  const int16* getData() const { return (int16*)data.data(); }
  int64 getDataSize() const { return width * height * 2; }

private:

  void reset();

  std::vector<byte> data; // elevation in meters

};

// Holds data with lifetime of a map
struct Map
{
  wchar_t name[64];
  wchar_t directoryPath[128];

  int64 widthInM = 0;
  int64 heightInM = 0;

  int16 minElevationInM = 0;
  int16 maxElevationInM = 0;

  Heightmap heightmap;

  float cameraZoomMin = 0.f;
  float cameraZoomMax = 0.f;
  float cameraNearPlane = 0.f;
  float cameraFarPlane = 0.f;

  bool tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);
};