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

  int16 minElevationInM = 0;
  int16 maxElevationInM = 0;
  float minElevationInKm = 0.f;
  float maxElevationInKm = 0.f;

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

  int64 widthInM = 1565430; // TODO: read these values from map.ini. Maybe using libconfini?
  int64 heightInM = 1878516;
  float widthInKm = 1565.430f;
  float heightInKm = 1878.516f;

  Heightmap heightmap;

  float cameraZoomMin;
  float cameraZoomMax;
  float cameraNearPlane;
  float cameraFarPlane;

  bool tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);
};