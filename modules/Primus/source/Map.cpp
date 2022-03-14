#include "Primus/Map.hpp"

#include "Primus/Primus.hpp"

#include "Core/Image.hpp"

#define MAP_DIRECTORY ASSET_DIRECTORY L"/maps"

bool Heightmap::tryLoad(const wchar_t* mapName)
{
  wchar_t mapFilePath[128];
  swprintf_s(mapFilePath, L"%ls/%ls/heightmap.png", MAP_DIRECTORY, mapName);

  image = readPng(mapFilePath);
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

  return true;
}

const uint16_t* Heightmap::getData() const
{
  return reinterpret_cast<uint16_t*>(image.data);
}

int64 Heightmap::getDataSize() const
{
  return image.height * image.width * 2;
}

bool Map::tryLoad(const wchar_t* mapName)
{
  return heightmap.tryLoad(mapName);
}