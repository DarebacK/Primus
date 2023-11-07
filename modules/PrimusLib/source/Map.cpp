#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Math.hpp"
#include "Core/Image.hpp"
#include "Core/File.hpp"
#include "Core/Config.hpp"
#include "Core/Asset.hpp"
#include "Core/String.hpp"

struct InitializeMapTaskData
{
  Map* map = nullptr;
  Ref<Config> config;
  float verticalFieldOfViewRadians = 0.f;
  float aspectRatio = 0.f;
};

DEFINE_TASK_BEGIN(initializeMap, InitializeMapTaskData)
{
  const Ref<Config>& config = taskData.config;
  Map& map = *taskData.map;
  const float verticalFieldOfViewRadians = taskData.verticalFieldOfViewRadians;
  const float aspectRatio = taskData.aspectRatio;

  map.minElevationInM = (int16)config->getInt("minElevationInM");
  map.maxElevationInM = (int16)config->getInt("maxElevationInM");
  map.widthInM = config->getInt("widthInM");
  map.heightInM = config->getInt("heightInM");

  map.cameraNearPlane = 1.f;

  // TODO: use meters as units in our coordinate system.
  map.cameraZoomMin = (map.maxElevationInM * 0.005f) + map.cameraNearPlane; // TODO: * 0.005f due to visual height multiplier. Put it in a map constant buffer and share it with the terrain vertex shader.
  const float horizontalFieldOfViewRadians = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  const float cameraYToFitMapHorizontally = cotan(horizontalFieldOfViewRadians / 2.f) * (map.widthInM / 2000.f);
  const float cameraYToFitMapVertically = cotan(verticalFieldOfViewRadians / 2.f) * (map.heightInM / 2000.f);
  map.cameraZoomMax = std::min(cameraYToFitMapHorizontally, cameraYToFitMapVertically);

  const float distanceToMaxZoomedOutHorizontalEdge = map.cameraZoomMax / cos(horizontalFieldOfViewRadians / 2.f);
  const float distanceToMaxZoomedOutVerticalEdge = map.cameraZoomMax / cos(verticalFieldOfViewRadians / 2.f);
  map.cameraFarPlane = std::max(distanceToMaxZoomedOutHorizontalEdge, distanceToMaxZoomedOutVerticalEdge); // TODO: adjust far clipping plane distance according to min elevation + current zoom level.
}
DEFINE_TASK_END

Ref<TaskEvent> Map::initializeAsync(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  TRACE_SCOPE();

  assetDirectory.initialize(mapDirectoryPath);

  heightmap = assetDirectory.findAsset<Texture2D>(L"heightmap.s16");
  colormap = assetDirectory.findAsset<Texture2D>(L"colormap.dds");

  Ref<Config> config = assetDirectory.findAsset<Config>(L"map.cfg");
  ensureTrue(config.isValid(), {});

  InitializeMapTaskData* taskData = new InitializeMapTaskData();
  taskData->map = this;
  taskData->config = std::move(config);
  taskData->verticalFieldOfViewRadians = verticalFieldOfViewRadians;
  taskData->aspectRatio = aspectRatio;

  return taskManager.schedule(initializeMap, taskData, ThreadType::Worker, &taskData->config->initializedTaskEvent, 1);
}