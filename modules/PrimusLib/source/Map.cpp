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
};

DEFINE_TASK_BEGIN(initializeMap, InitializeMapTaskData)
{
  const Ref<Config>& config = taskData.config;
  Map& map = *taskData.map;

  map.minElevationInM = (int16)config->getInt("minElevationInM");
  map.maxElevationInM = (int16)config->getInt("maxElevationInM");
  map.maxElevationInKm = map.maxElevationInM / 1000.f;
  map.widthInM = config->getInt("widthInM");
  map.widthInKm = map.widthInM / 1000.f;
  map.heightInM = config->getInt("heightInM");
  map.heightInKm = map.heightInM / 1000.f;
  map.visualHeightMultiplier = config->getFloat("visualHeightMultiplier");
  map.visualHeightMultiplierInverse = 1.f / map.visualHeightMultiplier;
}
DEFINE_TASK_END

Ref<TaskEvent> Map::initializeAsync(const wchar_t* mapDirectoryPath)
{
  TRACE_SCOPE();

  assetDirectory.initialize(mapDirectoryPath);

  heightmap = assetDirectory.findAsset<Texture2D>(L"heightmap");
  colormap = assetDirectory.findAsset<Texture2D>(L"colormap");

  Ref<Config> config = assetDirectory.findAsset<Config>(L"map");
  ensureTrue(config.isValid(), {});

  AssetDirectoryRef combinedTilesDirectory = assetDirectory.findSubdirectory(L"combinedTiles");
  combinedTilesDirectory.forEachAsset(AssetType::StaticMesh, [&](Asset* asset) {
    terrainTiles.emplace_back((StaticMesh*)asset);
  });

  InitializeMapTaskData* taskData = new InitializeMapTaskData();
  taskData->map = this;
  taskData->config = std::move(config);

  return schedule(initializeMap, taskData, ThreadType::Worker, &taskData->config->initializedTaskEvent, 1);
}

bool Map::isPointInside(const Vec3f& pointInWorldSpace) const
{
  return pointInWorldSpace.x >= 0.f && pointInWorldSpace.x <= widthInKm &&
    pointInWorldSpace.z >= 0.f && pointInWorldSpace.z <= heightInKm;
}