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

bool EditorMap::tryFixLandElevation()
{
  HttpDownloader downloader{ internet.get(), L"api.maptiler.com" };

  WebpReader webpReader;

  const int64 widthInTiles = heightmapTileXMax - heightmapTileXMin + 1;
  const int64 widthInPixels = widthInTiles * heightmapTileSize;
  const int64 heightInTiles = heightmapTileYMax - heightmapTileYMin + 1;
  const int64 heightInPixels = heightmapTileSize * heightInTiles;

  std::vector<byte> contentBuffer;
  for (int32 tileY = heightmapTileYMin; tileY <= heightmapTileYMax; tileY++)
  {
    const int64 tileYIndex = tileY - heightmapTileYMin;

    for (int32 tileX = heightmapTileXMin; tileX <= heightmapTileXMax; tileX++)
    {
      const int64 tileXIndex = tileX - heightmapTileXMin;

      wchar_t url[128];
      swprintf_s(url, L"tiles/hillshade/%hd/%hd/%hd.webp?key=jv0AKvuMCw6M6sWGYF8M", heightmapTileZoom, tileX, tileY);

      printf("Downloading %S\n", url);

      downloader.tryDownloadImage(url, contentBuffer);

      Image image = webpReader.read(contentBuffer.data(), int64(contentBuffer.size()), PixelFormat::RGBA);
      if (!image.data)
      {
        logError("Failed to fix land elevation: failed to read WebP hillshade data.");
        return false;
      }

      const byte* readDataIterator = image.data;
      for (int64 y = 0; y < image.height; ++y)
      {
        for (int64 x = 0; x < image.width; ++x)
        {
          uint8 r = *readDataIterator++;
          uint8 g = *readDataIterator++;
          uint8 b = *readDataIterator++;
          uint8 a = *readDataIterator++;
          int64 heightmapIndex = tileYIndex*widthInPixels*heightmapTileSize + tileXIndex*heightmapTileSize + y*widthInPixels + x;
          int16& heightmapValue = heightmap.data[heightmapIndex];
          if (a > 0 && heightmapValue <= 0)
          {
            heightmapValue = 1;
          }
          else if(a == 0 && heightmapValue > 0)
          {
            heightmapValue = 0;
          }
        }
      }
    }
  }

  //std::vector<uint16> heightmapToSerialize;
  //heightmapToSerialize.resize(heightmap.getDataSize());

  //for (int64 i = 0; i < heightmap.getDataSize() / 2; ++i)
  //{
  //  heightmapToSerialize[i] = nativeToBigEndian(uint16(heightmap.data[i] + 11000));
  //}

  //if (!writePngGrayscaleBigEndian("heightmap_fixed.png", reinterpret_cast<byte*>(heightmapToSerialize.data()), widthInPixels, heightInPixels, 1, 16))
  //{
  //  logError("Failed to write png data.");
  //  return false;
  //}

  return true;
}