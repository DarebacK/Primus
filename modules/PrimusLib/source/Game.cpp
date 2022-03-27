#include "Primus/Game.hpp"

#include "Primus/Map.hpp"

static constexpr float verticalFieldOfViewRadians = degreesToRadians(74.f);
static constexpr float cameraMoveSpeed = 0.03f;
static constexpr float cameraZoomSpeed = 0.1f;

Map currentMap;

bool Game::tryInitialize(Frame& firstFrame, D3D11Renderer& renderer)
{
  if (!currentMap.tryLoad(L"italy", verticalFieldOfViewRadians, firstFrame.aspectRatio))
  {
    return false;
  }

  if (!renderer.tryLoadMap(currentMap))
  {
    return false;
  }

  firstFrame.camera.position.x = currentMap.widthInMeters / 2.f;
  firstFrame.camera.position.y = currentMap.cameraZoomMax;
  firstFrame.camera.position.z = currentMap.heightInMeters / 2.f;

  return true;
}

void Game::update(const Frame& lastFrame, Frame& nextFrame, D3D11Renderer& renderer)
{
  nextFrame.camera.position = lastFrame.camera.position;

  // i.e. how many meters/units it is from camera center to the intersection of it's frustum with the map plane.
  float cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.position.y;

  const float currentCameraSpeed = cameraMoveSpeed * cameraEdgeVerticalOffsetFromPosition;
  nextFrame.camera.position.x += currentCameraSpeed * nextFrame.input.keyboard.d.isDown - currentCameraSpeed * nextFrame.input.keyboard.a.isDown;
  nextFrame.camera.position.z += currentCameraSpeed * nextFrame.input.keyboard.w.isDown - currentCameraSpeed * nextFrame.input.keyboard.s.isDown;

  if (nextFrame.input.mouse.dWheel > 0.f)
  {
    for (float i = 0.f; i < nextFrame.input.mouse.dWheel; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.position.y;
      const float zoomInSpeed = cotan(verticalFieldOfViewRadians / 2.f) * cameraZoomSpeed * cameraEdgeVerticalOffsetFromPosition;
      nextFrame.camera.position.y += -zoomInSpeed;
    }
  }
  else if (nextFrame.input.mouse.dWheel < 0.f)
  {
    for (float i = nextFrame.input.mouse.dWheel; i < 0.f; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.position.y;
      const float zoomOutSpeed = cotan(verticalFieldOfViewRadians / 2.f) * ((1.f / (1.f - cameraZoomSpeed)) - 1.f) * cameraEdgeVerticalOffsetFromPosition;
      nextFrame.camera.position.y += zoomOutSpeed;
    }
  }

  // Clamp camera to map bounds
  nextFrame.camera.position.y = std::clamp(nextFrame.camera.position.y, currentMap.cameraZoomMin, currentMap.cameraZoomMax);
  const float horizontalFieldOfView = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, nextFrame.aspectRatio);
  Vec2f cameraEdgeOffsetFromPosition = { tan(horizontalFieldOfView / 2.f) * nextFrame.camera.position.y, tan(verticalFieldOfViewRadians / 2.f) * nextFrame.camera.position.y };
  const float cameraLeftEdge = nextFrame.camera.position.x - cameraEdgeOffsetFromPosition.x;
  if (cameraLeftEdge < 0.f)
  {
    nextFrame.camera.position.x -= cameraLeftEdge;
  }
  else
  {
    const float cameraRightEdge = nextFrame.camera.position.x + cameraEdgeOffsetFromPosition.x;
    if (cameraRightEdge > currentMap.widthInMeters)
    {
      nextFrame.camera.position.x -= cameraRightEdge - currentMap.widthInMeters;
    }
  }
  const float cameraDownEdge = nextFrame.camera.position.z - cameraEdgeOffsetFromPosition.y;
  if (cameraDownEdge < 0.f)
  {
    nextFrame.camera.position.z -= cameraDownEdge;
  }
  else
  {
    const float cameraUpEdge = nextFrame.camera.position.z + cameraEdgeOffsetFromPosition.y;
    if (cameraUpEdge > currentMap.heightInMeters)
    {
      nextFrame.camera.position.z -= cameraUpEdge - currentMap.heightInMeters;
    }
  }

  const float cameraZoom = nextFrame.camera.position.y; // TODO: this isn't true anymore as angled camera zoom doesn't equal to y.
  const float cameraAngleInDegrees = lerp(-35.f, 0, cameraZoom / (currentMap.cameraZoomMax - currentMap.cameraZoomMin));
  const Mat3f cameraRotation = Mat3f::rotationX(degreesToRadians(cameraAngleInDegrees));
  const Vec3f cameraDirection = Vec3f{ 0.f, -1.f, 0.f } * cameraRotation;
  const Vec3f cameraUpVector = Vec3f{ 0.f, 0.f, 1.f} * cameraRotation;
  nextFrame.camera.view = Mat4x3f::lookTo(nextFrame.camera.position, cameraDirection, cameraUpVector);
  nextFrame.camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, nextFrame.aspectRatio, currentMap.cameraNearPlane, currentMap.cameraFarPlane);
  nextFrame.camera.viewProjection = nextFrame.camera.view * nextFrame.camera.projection;

  renderer.render(nextFrame, currentMap);
}