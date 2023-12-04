#include "Primus/Game.hpp"

#include "Primus/Primus.hpp"
#include "Primus/Map.hpp"

#define MAPS_DIRECTORY L"maps"

Map currentMap;

bool Game::tryInitialize(Frame& firstFrame, D3D11Renderer& renderer)
{
  TRACE_SCOPE();

  wchar_t mapDirectoryPath[256];
  swprintf_s(mapDirectoryPath, L"%ls/italy", MAPS_DIRECTORY);
  Ref<TaskEvent> mapInitializedEvent = currentMap.initializeAsync(mapDirectoryPath);
  if(!mapInitializedEvent.isValid())
  {
    return false;
  }

  if (!renderer.tryLoadMap(currentMap))
  {
    logError("Renderer failed to load map %ls", mapDirectoryPath);
    return false;
  }

  while(!mapInitializedEvent->isComplete());

  firstFrame.updateCameraZoomLimits(currentMap);
  firstFrame.camera.endPosition.x = currentMap.widthInM / 2000.f;
  firstFrame.camera.endPosition.y = firstFrame.camera.zoomMax;
  firstFrame.camera.endPosition.z = currentMap.heightInM / 2000.f;
  firstFrame.camera.currentPosition = firstFrame.camera.endPosition;
  firstFrame.updateCamera(currentMap, firstFrame.camera.currentPosition, firstFrame.camera.endPosition);

  return true;
}

void Game::update(const Frame& lastFrame, Frame& nextFrame, D3D11Renderer& renderer)
{
  TRACE_SCOPE();

  nextFrame.updateCamera(currentMap, lastFrame.camera.currentPosition, lastFrame.camera.endPosition);

  {
    TRACE_SCOPE("castRayToTerrain");

    // TODO: refactor ray casting into it's own function.
    Vec3f cursorPositionInClipSpace{ -0.5f + nextFrame.input.cursorPositionInClientSpaceNormalized.x, 0.5f - nextFrame.input.cursorPositionInClientSpaceNormalized.y, 0.f };
    cursorPositionInClipSpace *= 2.f;
    Vec4f cursorPositionInWorldSpace = toVec4f(cursorPositionInClipSpace, 1.f)  * nextFrame.camera.viewProjectionInverse;
    cursorPositionInWorldSpace /= cursorPositionInWorldSpace.w;
    Vec3f cursorRayDirection = normalized(toVec3f(cursorPositionInWorldSpace) - nextFrame.camera.currentPosition);
    const Vec3f mapPlane = { 0.f, 1.f, 0.f };
    Vec3f intersectionInWorldSpace;
    // We don't need to test from the cursor position as it's much higher than the highest place on the map.
    // So move the start when y == maxElevationInKm + a small offset.
    Vec3f rayOrigin;
    rayOrigin.y = std::min(cursorPositionInWorldSpace.y, currentMap.maxElevationInM * currentMap.visualHeightMultiplier + 1.f);
    float rayOriginT = (rayOrigin.y - cursorPositionInWorldSpace.y) / cursorRayDirection.y;
    rayOrigin.x = cursorPositionInWorldSpace.x + rayOriginT * cursorRayDirection.x;
    rayOrigin.z = cursorPositionInWorldSpace.z + rayOriginT * cursorRayDirection.z;
    if(ensure(rayIntersectsPlane(rayOrigin, cursorRayDirection, mapPlane, 0.f, intersectionInWorldSpace)))
    {
      if(currentMap.isPointInside(intersectionInWorldSpace))
      {
        Vec2f intersectionUv;
        intersectionUv.x = intersectionInWorldSpace.x / currentMap.widthInKm;
        intersectionUv.y = (currentMap.heightInKm - intersectionInWorldSpace.z) / currentMap.heightInKm;
        const Vec2i intersectionHeightmapTexel = currentMap.heightmap->uvToTexel(intersectionUv);;

        Vec2f rayOriginUv;
        rayOriginUv.x = rayOrigin.x / currentMap.widthInKm;
        rayOriginUv.y = (currentMap.heightInKm - rayOrigin.z) / currentMap.heightInKm;
        const Vec2i rayOriginHeightmapTexel = currentMap.heightmap->uvToTexel(rayOriginUv);

        //logInfo("World: {%f %f %f} -> {%f %f %f}", rayOrigin.x, rayOrigin.y, rayOrigin.z,
        //  intersectionInWorldSpace.x, intersectionInWorldSpace.y, intersectionInWorldSpace.z);
        //logInfo("UV: {%f %f} -> {%f %f}", rayOriginUv.x, rayOriginUv.y, intersectionUv.x, intersectionUv.y);
        //logInfo("Texel: {%lld %lld} -> {%lld %lld}", rayOriginHeightmapTexel.x, rayOriginHeightmapTexel.y, intersectionHeightmapTexel.x, intersectionHeightmapTexel.y);

        // Rasterization from http://members.chello.at/~easyfilter/Bresenham.pdf
        int64 x = rayOriginHeightmapTexel.x;
        int64 y = rayOriginHeightmapTexel.y;
        int64 dx = std::abs(intersectionHeightmapTexel.x - rayOriginHeightmapTexel.x);
        int64 sx = rayOriginHeightmapTexel.x < intersectionHeightmapTexel.x ? 1 : -1;
        int64 dy = -abs(intersectionHeightmapTexel.y - rayOriginHeightmapTexel.y);
        int64 sy = rayOriginHeightmapTexel.y < intersectionHeightmapTexel.y ? 1 : -1;
        int64 error = dx + dy;
        int64 error2;
        float t = 0.f;
        for(;;) {
          const float yWorldSpace = rayOrigin.y + t * cursorRayDirection.y;

          if(currentMap.heightmap->isTexelInside(x, y))
          {
            // TODO: we should test exactly on the ray point vs the height inside the triangle, not nearest texel
            const float heightInM = std::max(float(currentMap.heightmap->sample<int16>(x, y)), 0.f);
            const float visualHeightInKm = heightInM * currentMap.visualHeightMultiplier;
            if(yWorldSpace <= visualHeightInKm)
            {
              break;
            }
          }

          error2 = 2 * error;
          if(error2 >= dy) {
            if(x == intersectionHeightmapTexel.x) break;
            error += dy; x += sx;
            const float xWorldSpace = (x / float(currentMap.heightmap->width - 1)) * currentMap.widthInKm;
            t = (xWorldSpace - rayOrigin.x) / cursorRayDirection.x;
          }
          if(error2 <= dx) {
            if(y == intersectionHeightmapTexel.y) break;
            error += dx; y += sy;
            const float zWorldSpace = ((currentMap.heightmap->height - 1 - y) / float(currentMap.heightmap->height - 1)) * currentMap.heightInKm;
            t = (zWorldSpace - rayOrigin.z) / cursorRayDirection.z;
          }
        }

        const float heightInKm = std::max(float(currentMap.heightmap->sample<int16>(x, y)), 0.f) / 1000.f;
        //logInfo("Hit texel = {%lld %lld}, height = %fkm", x, y, heightInKm);
      }
    }
  }

  renderer.beginRender();
  renderer.render(nextFrame, currentMap);
  renderer.endRender();
}