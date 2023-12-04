#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"
#include "Core/Task.hpp"
#include "Core/Asset.hpp"

struct Vec3f;

// Holds data with lifetime of a map
struct Map
{
  AssetDirectoryRef assetDirectory;

  int64 widthInM = 0;
  float widthInKm = 0.f;
  int64 heightInM = 0;
  float heightInKm = 0.f;
  int16 minElevationInM = 0;
  int16 maxElevationInM = 0;
  float maxElevationInKm = 0.f;
  float visualHeightMultiplier = 1.f;
  float visualHeightMultiplierInverse = 1.f;

  Ref<Texture2D> heightmap;
  Ref<Texture2D> colormap;

  std::vector<Ref<StaticMesh>> terrainTiles;

  Ref<TaskEvent> initializeAsync(const wchar_t* mapDirectoryPath);

  bool isPointInside(const Vec3f& pointInWorldSpace) const;
};