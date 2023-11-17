#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"
#include "Core/Task.hpp"
#include "Core/Asset.hpp"

// Holds data with lifetime of a map
struct Map
{
  AssetDirectoryRef assetDirectory;

  int64 widthInM = 0;
  int64 heightInM = 0;
  int16 minElevationInM = 0;
  int16 maxElevationInM = 0;
  float visualHeightMultiplier = 1.f;
  float visualHeightMultiplierInverse = 1.f;

  Ref<Texture2D> heightmap;
  Ref<Texture2D> colormap;

  float cameraZoomMin = 0.f;
  float cameraZoomMax = 0.f;
  float cameraNearPlane = 0.f;
  float cameraFarPlane = 0.f;

  Ref<TaskEvent> initializeAsync(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);
};