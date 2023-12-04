#include "EditorMap.hpp"

#include "Core/String.hpp"

#include "Internet.hpp"

bool EditorMap::tryLoad(const wchar_t* mapDirectoryPath)
{
  Ref<TaskEvent> mapInitializedEvent = Map::initializeAsync(mapDirectoryPath);

  wcscpy_s(path, mapDirectoryPath);

  const int64 lengthUntilMapName = getLengthUntilLastSlash(mapDirectoryPath) + 1;
  wcscpy_s(name, mapDirectoryPath + lengthUntilMapName);

  // TODO: don't wait for it, keep the UI responsive 
  mapInitializedEvent->waitForCompletion();

  return true;
}
