#pragma once

#include "EditorMap.hpp"

#include "Internet.hpp"

void downloadHeightmap(HINTERNET internet, const char* outputFileName);
void exportHeightmapToObj(const EditorMap& map, const wchar_t* path);
void exportHeightmapToObjTiles(const EditorMap& map, const wchar_t* path);
