#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Math.hpp"
#include "Core/Image.hpp"
#include "Core/File.hpp"
#include "Core/Config.hpp"
#include "Core/Asset.hpp"
#include "Core/String.hpp"

bool Map::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  TRACE_SCOPE();

  AssetDirectoryRef assetDirectory{ mapDirectoryPath };

  // TODO: This sleep just makes "sure" that Config is initialized when we trying to look for it. Instead continue with the asynchronization here: make a map load task that has prerequisite of the assetDirectory finished loading event.
  Sleep(100);

  // TODO: Get rid of passing the file extension, it's an implementation detail after all.
  Ref<Config> config = assetDirectory.findAsset<Config>(L"map.cfg");
  minElevationInM = config->getInt("minElevationInM");
  maxElevationInM = config->getInt("maxElevationInM");
  widthInM = config->getInt("widthInM");
  heightInM = config->getInt("heightInM");

  heightmap = assetDirectory.findAsset<Texture2D>(L"heightmap.s16");
  colormap = assetDirectory.findAsset<Texture2D>(L"colormap.dds");

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