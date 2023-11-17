#pragma once

#include "Core/Asset.hpp"

#include "Internet.hpp"

void downloadHeightmap(HINTERNET internet, const char* outputFileName);
void writeHeightmapToObj(const Ref<Texture2D>& heightmap);
