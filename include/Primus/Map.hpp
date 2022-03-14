#pragma once

#include "Core/Core.hpp"
#include "Core/Image.hpp"

class Heightmap
{
public:

  Heightmap() = default;
  Heightmap(const Heightmap& other) = delete;
  Heightmap(const Heightmap&& other) = delete;
  ~Heightmap() = default;

  bool tryLoad(const wchar_t* mapName);

  const uint16_t* getData() const;
  int64 getDataSize() const;

private:

  PngReadResult image;
};

// Holds data with lifetime of a map
struct Map
{
  Heightmap heightmap;

  bool tryLoad(const wchar_t* mapName);
};