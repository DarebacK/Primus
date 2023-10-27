#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Math.hpp"
#include "Core/Image.hpp"
#include "Core/File.hpp"
#include "Core/Config.hpp"
#include "Core/Asset.hpp"
#include "Core/String.hpp"

Heightmap::Heightmap(Heightmap&& other) noexcept
{
  using std::swap;

  swap(data, other.data);
  swap(width, other.width);
  swap(height, other.height);
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
  swprintf_s(mapFilePath, L"%ls\\heightmap.s16", mapDirectoryPath);

  if (!tryReadEntireFile(mapFilePath, data))
  {
    logError("Failed to read heightmap file %ls", mapFilePath);
    return false;
  }

  // TODO: read from a meta file
  width = 2560;
  height = 3072;

  return true;
}

void Heightmap::reset()
{
  data.clear();
  width = 0;
  height = 0;
}

bool Map::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  TRACE_SCOPE();

  wcscpy_s(directoryPath, mapDirectoryPath);
  AssetDirectoryRef assetDirectory{ directoryPath };

  // TODO: Remove this after manual asset loading is replaced.
  int64 directoryPathLength = combine(ASSET_DIRECTORY, L'/', mapDirectoryPath, directoryPath);
  directoryPath[directoryPathLength] = L'\0';

  // TODO: Get rid of passing the file extension, it's an implementation detail after all.
  Ref<Config> config{ assetDirectory.findAsset<Config>(L"map.cfg") };
  minElevationInM = config->getInt("minElevationInM");
  maxElevationInM = config->getInt("maxElevationInM");
  widthInM = config->getInt("widthInM");
  heightInM = config->getInt("heightInM");

  // TODO: replace manual asset loading with directory
  if (!heightmap.tryLoad(directoryPath))
  {
    return false;
  }

  cameraNearPlane = 1.f;

  // TODO: use meters as units in our coordinate system.
  cameraZoomMin = (maxElevationInM * 0.005f) + cameraNearPlane; // TODO: * 0.005f due to visual height multiplier. Put it in a map constant buffer and share it with the terrain vertex shader.
  const float horizontalFieldOfViewRadians = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  const float cameraYToFitMapHorizontally = cotan(horizontalFieldOfViewRadians / 2.f) * (widthInM / 2000.f);
  const float cameraYToFitMapVertically = cotan(verticalFieldOfViewRadians / 2.f) * (heightInM / 2000.f);
  cameraZoomMax = std::min(cameraYToFitMapHorizontally, cameraYToFitMapVertically);

  const float distanceToMaxZoomedOutHorizontalEdge = cameraZoomMax / cos(horizontalFieldOfViewRadians / 2.f);
  const float distanceToMaxZoomedOutVerticalEdge = cameraZoomMax / cos(verticalFieldOfViewRadians / 2.f);
  cameraFarPlane = std::max(distanceToMaxZoomedOutHorizontalEdge, distanceToMaxZoomedOutVerticalEdge); // TODO: adjust far clipping plane distance according to min elevation + current zoom level.

  return true;
}