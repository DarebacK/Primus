#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"
#include "Core/Task.hpp"
#include "Core/Asset.hpp"

// Holds data with lifetime of a map
struct Map
{
  wchar_t name[64];

  int64 widthInM = 0;
  int64 heightInM = 0;

  int16 minElevationInM = 0;
  int16 maxElevationInM = 0;

  Ref<Texture2D> heightmap;
  Ref<Texture2D> colormap;

  float cameraZoomMin = 0.f;
  float cameraZoomMax = 0.f;
  float cameraNearPlane = 0.f;
  float cameraFarPlane = 0.f;

  bool tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio);
};