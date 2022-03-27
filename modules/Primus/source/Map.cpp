#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Math.hpp"
#include "Core/Image.hpp"

#define MAPS_DIRECTORY ASSET_DIRECTORY L"/maps"

Heightmap::~Heightmap()
{
  reset();
}

bool Heightmap::tryLoad(const wchar_t* mapDirectory)
{
  reset();

  wchar_t mapFilePath[128];
  swprintf_s(mapFilePath, L"%lsheightmap.png", mapDirectory);

  PngReadResult image = readPng(mapFilePath);
  if (!image.data || !image.width || !image.height)
  {
    logError("Failed to read heightmap png file %ls", mapFilePath);
    return false;
  }

  if (image.channelCount != 1)
  {
    logError("Heightmap file %ls isn't grayscale.", mapFilePath);
    return false;
  }

  data = reinterpret_cast<int16*>(image.data);
  image.data = nullptr;
  width = int32(image.width);
  height = int32(image.height);

  taskScheduler.parallelFor(0, image.height, [this](int64 i, int64 threadIndex) {
    uint16* const grayscaleDataRow = reinterpret_cast<uint16*>(data) + i * width;
    int16* const elevationDataRow = reinterpret_cast<int16*>(grayscaleDataRow);
    for (int64 x = 0; x < width; x++)
    {
      elevationDataRow[x] = static_cast<int16>(static_cast<int32>(bigEndianToNative(grayscaleDataRow[x])) - 11000);
    }
  });

  return true;
}

void Heightmap::reset()
{
  if (data)
  {
    free(data);
    data = nullptr;
  }
  width = 0;
  height = 0;
}

bool Map::tryLoad(const wchar_t* mapName, float verticalFieldOfViewRadians, float aspectRatio)
{
  wcscpy_s(name, mapName);

  swprintf_s(directoryPath, L"%ls/%ls/", MAPS_DIRECTORY, mapName);

  if (!heightmap.tryLoad(directoryPath))
  {
    return false;
  }

  cameraZoomMin = 2.f;
  const float horizontalFieldOfViewRadians = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  const float cameraYToFitMapHorizontally = cotan(horizontalFieldOfViewRadians / 2.f) * (widthInMeters / 2.f);
  const float cameraYToFitMapVertically = cotan(verticalFieldOfViewRadians / 2.f) * (heightInMeters / 2.f);
  cameraZoomMax = std::min(cameraYToFitMapHorizontally, cameraYToFitMapVertically);

  cameraNearPlane = 1.f;
  const float distanceToMaxZoomedOutHorizontalEdge = cameraZoomMax / cos(horizontalFieldOfViewRadians / 2.f);
  const float distanceToMaxZoomedOutVerticalEdge = cameraZoomMax / cos(verticalFieldOfViewRadians / 2.f);
  cameraFarPlane = std::max(distanceToMaxZoomedOutHorizontalEdge, distanceToMaxZoomedOutVerticalEdge); // TODO: adjust far clipping plane distance according to min elevation + current zoom level.

  return true;
}