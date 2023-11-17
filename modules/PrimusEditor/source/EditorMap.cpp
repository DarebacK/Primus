#include "EditorMap.hpp"

#include "Core/String.hpp"

#include "Internet.hpp"

bool EditorMap::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  Ref<TaskEvent> mapInitializedEvent = Map::initializeAsync(mapDirectoryPath, verticalFieldOfViewRadians, aspectRatio);

  const int64 lengthUntilMapName = getLengthUntilLastSlash(mapDirectoryPath) + 1;
  wcscpy_s(name, mapDirectoryPath + lengthUntilMapName);

  // TODO: don't wait for it, keep the UI responsive 
  mapInitializedEvent->waitForCompletion();

  return true;
}
