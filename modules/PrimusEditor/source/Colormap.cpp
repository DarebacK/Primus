#include "Colormap.hpp"

#include "Core/Image.hpp"
#include "Core/Task.hpp"

#include <vector>
#include <memory>
#include <fstream>

namespace {
  constexpr int tileSize = 256;
  #define TILE_ZOOM 10

  constexpr int64 tileXMin = 528;
  constexpr int64 tileXMax = 567;
  constexpr int64 widthInTiles = tileXMax - tileXMin + 1;
  constexpr int64 widthInPixels = widthInTiles * tileSize;
  constexpr size_t writePitch = widthInPixels * 3;
  constexpr int64 tileYMin = 360;
  constexpr int64 tileYMax = 407;
  constexpr int64 heightInTiles = tileYMax - tileYMin + 1;
  constexpr int64 heightInPixels = tileSize * heightInTiles;
  constexpr int64 tileCount = widthInTiles * heightInTiles;
  constexpr size_t bytesPerPixel = 3;

  // https://api.maptiler.com/tiles/satellite-mediumres/{z}/{x}/{y}.jpg?key=jv0AKvuMCw6M6sWGYF8M
  const wchar_t* serverUrl = L"api.maptiler.com";
  const wchar_t* requestUrlBegin = L"tiles/satellite-mediumres/" WSTRINGIFY_DEFINE(TILE_ZOOM) L"/";
  const wchar_t* requestUrlEnd = L".jpg?key=jv0AKvuMCw6M6sWGYF8M";
}

void downloadColormap(HINTERNET internet, const char* outputFileName)
{
  HttpDownloader downloader{ internet, serverUrl };

  Image colormap;
  colormap.data = static_cast<byte*>(malloc(tileCount * tileSize * tileSize * bytesPerPixel));
  if (!colormap.data)
  {
    logError("Failed to allocate memory for to be downloaded color map.");
    return;
  }
  colormap.width = widthInPixels;
  colormap.height = heightInPixels;
  colormap.pixelFormat = PixelFormat::BGR;

  std::vector<std::vector<byte>> contentBuffers;
  contentBuffers.resize(tileCount);

  for (int32 tileY = tileYMin; tileY <= tileYMax; tileY++)
  {
    const int64 tileYIndex = tileY - tileYMin;

    for (int32 tileX = tileXMin; tileX <= tileXMax; tileX++)
    {
      const int64 tileXIndex = tileX - tileXMin;

      std::vector<byte>& contentBuffer = contentBuffers[tileYIndex * widthInTiles + tileXIndex];

      wchar_t url[128];
      swprintf_s(url, L"%ls/%d/%d%ls", requestUrlBegin, tileX, tileY, requestUrlEnd);

      logInfo("Downloading %S\n", url);

      if (!downloader.tryDownloadImage(url, contentBuffer))
      {
        logError("Failed to download color map image tile %d %d %ls", tileX, tileY, WSTRINGIFY_DEFINE(TILE_ZOOM));
        return;
      }
    }
  }

  std::vector<JpegReader> jpegReaders;
  jpegReaders.resize(getWorkerCount() + 1);

  parallelFor(0, tileCount, [&contentBuffers, &jpegReaders, &colormap](int64 i, int64 threadIndex) {
    const int64 tileXIndex = i % widthInTiles;
    const int64 tileYIndex = i / widthInTiles;

    std::vector<byte>& contentBuffer = contentBuffers[tileYIndex * widthInTiles + tileXIndex];

    Image tileImage = jpegReaders[threadIndex].read(contentBuffer.data(), contentBuffer.size(), colormap.pixelFormat);
    if (!tileImage.data)
    {
      logError("Failed to read downloaded color map image tile %lld %lld %ls", tileXMin + tileXIndex, tileXMax + tileYIndex, WSTRINGIFY_DEFINE(TILE_ZOOM));
      return;
    }

    colormap.chromaSubsampling = tileImage.chromaSubsampling;

    const size_t writeOffset = tileYIndex * tileSize * writePitch + tileXIndex * tileSize * bytesPerPixel;
    const size_t readPitch = tileImage.width * bytesPerPixel;
    for (int64 y = 0; y < tileImage.height; ++y)
    {
      memcpy(colormap.data + writeOffset + y * writePitch, tileImage.data + y * readPitch, tileImage.width * bytesPerPixel);
    }
  });

  ImageEncoder encoder;
  Image colormapBc1 = encoder.ToBC1Parallel(colormap, 100);

  char outputFilePath[MAX_PATH];
  sprintf_s(outputFilePath, "%s.dds", outputFileName);
  writeAsDds(colormapBc1, outputFilePath);
}