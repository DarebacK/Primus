#pragma once

#include "Core/Asset.hpp"

#include "Internet.hpp"

void downloadHeightmap(HINTERNET internet, const char* outputFileName);
void exportHeightmapToObj(const Texture2D* heightmap, const wchar_t* path);
void exportHeightmapToObjTiles(const Texture2D* heightmap, const wchar_t* path);
