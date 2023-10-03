#include "Primus/Game.hpp"

#include "Primus/Primus.hpp"
#include "Primus/Map.hpp"

static constexpr float cameraMoveSpeed = 2.5f;
static constexpr float cameraZoomSpeed = 0.12f;

#define MAPS_DIRECTORY ASSET_DIRECTORY L"/maps"

Map currentMap;

bool Game::tryInitialize(Frame& firstFrame, D3D11Renderer& renderer)
{
  TRACE_SCOPE();

  wchar_t mapDirectoryPath[256];
  swprintf_s(mapDirectoryPath, L"%ls/italy", MAPS_DIRECTORY);
  if (!currentMap.tryLoad(mapDirectoryPath, verticalFieldOfViewRadians, firstFrame.aspectRatio))
  {
    logError("Failed to load map %ls", mapDirectoryPath);
    return false;
  }

  if (!renderer.tryLoadMap(currentMap))
  {
    logError("Renderer failed to load map %ls", mapDirectoryPath);
    return false;
  }

  firstFrame.camera.endPosition.x = currentMap.widthInM / 2000.f;
  firstFrame.camera.endPosition.y = currentMap.cameraZoomMax;
  firstFrame.camera.endPosition.z = currentMap.heightInM / 2000.f;
  firstFrame.camera.currentPosition = firstFrame.camera.endPosition;

  return true;
}

void Game::update(const Frame& lastFrame, Frame& nextFrame, D3D11Renderer& renderer)
{
  nextFrame.camera.currentPosition = lastFrame.camera.currentPosition;
  nextFrame.camera.endPosition = lastFrame.camera.endPosition;

  // i.e. how many meters/units it is from camera center to the intersection of it's frustum with the map plane.
  float cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.endPosition.y;

  const float currentCameraSpeed = cameraMoveSpeed * cameraEdgeVerticalOffsetFromPosition * nextFrame.deltaTime;
  nextFrame.camera.endPosition.x += currentCameraSpeed * nextFrame.input.keyboard.d.isDown - currentCameraSpeed * nextFrame.input.keyboard.a.isDown;
  nextFrame.camera.endPosition.z += currentCameraSpeed * nextFrame.input.keyboard.w.isDown - currentCameraSpeed * nextFrame.input.keyboard.s.isDown;

  if (nextFrame.input.mouse.dWheel > 0.f)
  {
    for (float i = 0.f; i < nextFrame.input.mouse.dWheel; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.endPosition.y;
      const float zoomInSpeed = cotan(verticalFieldOfViewRadians / 2.f) * cameraZoomSpeed * cameraEdgeVerticalOffsetFromPosition;
      nextFrame.camera.endPosition.y += -zoomInSpeed;
    }
  }
  else if (nextFrame.input.mouse.dWheel < 0.f)
  {
    for (float i = nextFrame.input.mouse.dWheel; i < 0.f; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.endPosition.y;
      const float zoomOutSpeed = cotan(verticalFieldOfViewRadians / 2.f) * ((1.f / (1.f - cameraZoomSpeed)) - 1.f) * cameraEdgeVerticalOffsetFromPosition;
      nextFrame.camera.endPosition.y += zoomOutSpeed;
    }
  }

  // Clamp camera to map bounds
  nextFrame.camera.endPosition.y = std::clamp(nextFrame.camera.endPosition.y, currentMap.cameraZoomMin, currentMap.cameraZoomMax);
  const float horizontalFieldOfView = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, nextFrame.aspectRatio);
  Vec2f cameraEdgeOffsetFromPosition = { tan(horizontalFieldOfView / 2.f) * nextFrame.camera.endPosition.y, tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.endPosition.y };
  const float cameraLeftEdge = nextFrame.camera.endPosition.x - cameraEdgeOffsetFromPosition.x;
  if (cameraLeftEdge < 0.f)
  {
    nextFrame.camera.endPosition.x -= cameraLeftEdge;
  }
  else
  {
    const float cameraRightEdge = nextFrame.camera.endPosition.x + cameraEdgeOffsetFromPosition.x;
    if (cameraRightEdge > currentMap.widthInM / 1000.f)
    {
      nextFrame.camera.endPosition.x -= cameraRightEdge - currentMap.widthInM / 1000.f;
    }
  }
  const float cameraDownEdge = nextFrame.camera.endPosition.z - cameraEdgeOffsetFromPosition.y;
  if (cameraDownEdge < 0.f)
  {
    nextFrame.camera.endPosition.z -= cameraDownEdge;
  }
  else
  {
    const float cameraUpEdge = nextFrame.camera.endPosition.z + cameraEdgeOffsetFromPosition.y;
    if (cameraUpEdge > currentMap.heightInM / 1000.f)
    {
      nextFrame.camera.endPosition.z -= cameraUpEdge - currentMap.heightInM / 1000.f;
    }
  }

  const Vec3f remainingDistance = nextFrame.camera.endPosition - nextFrame.camera.currentPosition;
  Vec3f frameDelta = 10 * nextFrame.deltaTime * remainingDistance;
  nextFrame.camera.currentPosition += frameDelta;

  const float cameraZoom = nextFrame.camera.currentPosition.y; // TODO: this isn't true anymore as angled camera zoom doesn't equal to y.
  const float cameraAngleInDegrees = lerp(-35.f, 0.f, (cameraZoom - currentMap.cameraZoomMin) / (currentMap.cameraZoomMax - currentMap.cameraZoomMin));
  const Mat3f cameraRotation = Mat3f::rotationX(degreesToRadians(cameraAngleInDegrees));
  const Vec3f cameraDirection = Vec3f{ 0.f, -1.f, 0.f } * cameraRotation;
  const Vec3f cameraUpVector = Vec3f{ 0.f, 0.f, 1.f} * cameraRotation;
  nextFrame.camera.view = Mat4x3f::lookTo(nextFrame.camera.currentPosition, cameraDirection, cameraUpVector);
  nextFrame.camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, nextFrame.aspectRatio, currentMap.cameraNearPlane, currentMap.cameraFarPlane);
  nextFrame.camera.viewProjection = nextFrame.camera.view * nextFrame.camera.projection;

  renderer.beginRender();
  renderer.render(nextFrame, currentMap);
  renderer.endRender();
}