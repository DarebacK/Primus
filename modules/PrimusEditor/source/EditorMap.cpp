#include "EditorMap.hpp"

#include "Internet.hpp"

bool EditorMap::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  Ref<TaskEvent> mapInitializedEvent = Map::initializeAsync(mapDirectoryPath, verticalFieldOfViewRadians, aspectRatio);
  // TODO: don't wait for it, keep the UI responsive 
  mapInitializedEvent->waitForCompletion();

  return true;
}
