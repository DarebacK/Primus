#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Math.hpp"
#include "Core/Image.hpp"

Heightmap::Heightmap(Heightmap&& other) noexcept
{
  using std::swap;

  swap(data, other.data);
  swap(width, other.width);
  swap(height, other.height);

  swap(minElevationInM, other.minElevationInM);
  swap(maxElevationInM, other.maxElevationInM);
  swap(minElevationInKm, other.minElevationInKm);
  swap(maxElevationInKm, other.maxElevationInKm);
}

Heightmap::~Heightmap()
{
  reset();
}

bool Heightmap::tryLoad(const wchar_t* mapDirectoryPath)
{
  TRACE_SCOPE();

  reset();

  wchar_t mapFilePath[128];
  swprintf_s(mapFilePath, L"%ls\\heightmap.png", mapDirectoryPath);

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

  std::atomic<int16> atomicMinElevation = 0;
  std::atomic<int16> atomicMaxElevation = 0;
  taskManager.parallelFor(0, image.height, [this, &atomicMinElevation, &atomicMaxElevation](int64 i, int64 threadIndex) {
    int16 localMinElevation = std::numeric_limits<int16>::max();
    int16 localMaxElevation = std::numeric_limits<int16>::min();
    uint16* const grayscaleDataRow = reinterpret_cast<uint16*>(data) + i * width;
    int16* const elevationDataRow = reinterpret_cast<int16*>(grayscaleDataRow);
    for (int64 x = 0; x < width; x++)
    {
      elevationDataRow[x] = static_cast<int16>(bigEndianToNative(grayscaleDataRow[x]));
      localMinElevation = std::min(localMinElevation, elevationDataRow[x]);
      localMaxElevation = std::max(localMaxElevation, elevationDataRow[x]);
    }

    while (true)
    {
      int16 fetchedAtomicMinElevation = atomicMinElevation.load(std::memory_order_relaxed);
      localMinElevation = std::min(localMinElevation, fetchedAtomicMinElevation);
      if (atomicMinElevation.compare_exchange_strong(fetchedAtomicMinElevation, localMinElevation))
      {
        break;
      }
    }

    while (true)
    {
      int16 fetchedAtomicMaxElevation = atomicMaxElevation.load(std::memory_order_relaxed);
      localMaxElevation = std::max(localMaxElevation, fetchedAtomicMaxElevation);
      if (atomicMaxElevation.compare_exchange_strong(fetchedAtomicMaxElevation, localMaxElevation))
      {
        break;
      }
    }
  });

  // TODO: this data should be preprocessed
  minElevationInM = atomicMinElevation.load(std::memory_order_relaxed);
  maxElevationInM = atomicMaxElevation.load(std::memory_order_relaxed);
  minElevationInKm = minElevationInM / 1000.f;
  maxElevationInKm = maxElevationInM / 1000.f;

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

bool Map::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  TRACE_SCOPE();

  wcscpy_s(directoryPath, mapDirectoryPath);

  if (!heightmap.tryLoad(mapDirectoryPath))
  {
    return false;
  }

  cameraNearPlane = 1.f;

  cameraZoomMin = (heightmap.maxElevationInKm * 5.f) + cameraNearPlane; // TODO: * 5.f due to visual height multiplier. Put it in a map constant buffer and share it with the terrain vertex shader.
  const float horizontalFieldOfViewRadians = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  const float cameraYToFitMapHorizontally = cotan(horizontalFieldOfViewRadians / 2.f) * (widthInKm / 2.f);
  const float cameraYToFitMapVertically = cotan(verticalFieldOfViewRadians / 2.f) * (heightInKm / 2.f);
  cameraZoomMax = std::min(cameraYToFitMapHorizontally, cameraYToFitMapVertically);

  const float distanceToMaxZoomedOutHorizontalEdge = cameraZoomMax / cos(horizontalFieldOfViewRadians / 2.f);
  const float distanceToMaxZoomedOutVerticalEdge = cameraZoomMax / cos(verticalFieldOfViewRadians / 2.f);
  cameraFarPlane = std::max(distanceToMaxZoomedOutHorizontalEdge, distanceToMaxZoomedOutVerticalEdge); // TODO: adjust far clipping plane distance according to min elevation + current zoom level.

  return true;
}