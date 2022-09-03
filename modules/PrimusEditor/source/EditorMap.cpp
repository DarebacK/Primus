#include "EditorMap.hpp"

#include "Internet.hpp"

bool EditorMap::tryLoad(const wchar_t* mapDirectoryPath, float verticalFieldOfViewRadians, float aspectRatio)
{
  if (!Map::tryLoad(mapDirectoryPath, verticalFieldOfViewRadians, aspectRatio))
  {
    return false;
  }

  wchar_t colormapFilePath[256];
  wsprintfW(colormapFilePath, L"%ls\\colormap.jpg", mapDirectoryPath);

  JpegReader jpegReader;
  colormap.image = jpegReader.read(colormapFilePath, PixelFormat::RGB);

  if (!colormap.image.data)
  {
    logError("Failed to read colormap %ls", colormapFilePath);
    return false;
  }

  return true;
}
