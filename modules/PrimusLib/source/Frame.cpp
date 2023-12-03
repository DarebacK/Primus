#include "Primus/Frame.hpp"

#include "Primus/Primus.hpp"
#include "Primus/Map.hpp"

static constexpr float cameraMoveSpeed = 2.5f;
static constexpr float cameraZoomSpeed = 0.12f;

void Frame::updateCamera(const Map& currentMap, const Frame& lastFrame)
{
  TRACE_SCOPE();

  camera.currentPosition = lastFrame.camera.currentPosition;
  camera.endPosition = lastFrame.camera.endPosition;

  // i.e. how many meters/units it is from camera center to the intersection of it's frustum with the map plane.
  float cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * camera.endPosition.y;

  const float currentCameraSpeed = cameraMoveSpeed * cameraEdgeVerticalOffsetFromPosition * deltaTime;
  camera.endPosition.x += currentCameraSpeed * input.keyboard.d.isDown - currentCameraSpeed * input.keyboard.a.isDown;
  camera.endPosition.z += currentCameraSpeed * input.keyboard.w.isDown - currentCameraSpeed * input.keyboard.s.isDown;

  if(input.mouse.dWheel > 0.f)
  {
    for(float i = 0.f; i < input.mouse.dWheel; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * camera.endPosition.y;
      const float zoomInSpeed = cotan(verticalFieldOfViewRadians / 2.f) * cameraZoomSpeed * cameraEdgeVerticalOffsetFromPosition;
      camera.endPosition.y += -zoomInSpeed;
    }
  }
  else if(input.mouse.dWheel < 0.f)
  {
    for(float i = input.mouse.dWheel; i < 0.f; ++i)
    {
      cameraEdgeVerticalOffsetFromPosition = tan(verticalFieldOfViewRadians / 2.f) * camera.endPosition.y;
      const float zoomOutSpeed = cotan(verticalFieldOfViewRadians / 2.f) * ((1.f / (1.f - cameraZoomSpeed)) - 1.f) * cameraEdgeVerticalOffsetFromPosition;
      camera.endPosition.y += zoomOutSpeed;
    }
  }

  // Clamp camera to map bounds
  camera.endPosition.y = std::clamp(camera.endPosition.y, currentMap.cameraZoomMin, currentMap.cameraZoomMax);
  const float horizontalFieldOfView = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  Vec2f cameraEdgeOffsetFromPosition = { tan(horizontalFieldOfView / 2.f) * camera.endPosition.y, tan(verticalFieldOfViewRadians / 2.f) * camera.endPosition.y };
  const float cameraLeftEdge = camera.endPosition.x - cameraEdgeOffsetFromPosition.x;
  if(cameraLeftEdge < 0.f)
  {
    camera.endPosition.x -= cameraLeftEdge;
  }
  else
  {
    const float cameraRightEdge = camera.endPosition.x + cameraEdgeOffsetFromPosition.x;
    if(cameraRightEdge > currentMap.widthInKm)
    {
      camera.endPosition.x -= cameraRightEdge - currentMap.widthInKm;
    }
  }
  const float cameraDownEdge = camera.endPosition.z - cameraEdgeOffsetFromPosition.y;
  if(cameraDownEdge < 0.f)
  {
    camera.endPosition.z -= cameraDownEdge;
  }
  else
  {
    const float cameraUpEdge = camera.endPosition.z + cameraEdgeOffsetFromPosition.y;
    if(cameraUpEdge > currentMap.heightInKm)
    {
      camera.endPosition.z -= cameraUpEdge - currentMap.heightInKm;
    }
  }

  const Vec3f remainingDistance = camera.endPosition - camera.currentPosition;
  Vec3f frameDelta = 10 * deltaTime * remainingDistance;
  camera.currentPosition += frameDelta;

  const float cameraZoom = camera.currentPosition.y; // TODO: this isn't true anymore as angled camera zoom doesn't equal to y.
  const float cameraAngleInDegrees = lerp(-35.f, 0.f, (cameraZoom - currentMap.cameraZoomMin) / (currentMap.cameraZoomMax - currentMap.cameraZoomMin));
  const Mat3f cameraRotation = Mat3f::rotationX(degreesToRadians(cameraAngleInDegrees));
  const Vec3f cameraDirection = Vec3f{ 0.f, -1.f, 0.f } *cameraRotation;
  const Vec3f cameraUpVector = Vec3f{ 0.f, 0.f, 1.f } *cameraRotation;
  camera.view = Mat4x3f::lookTo(camera.currentPosition, cameraDirection, cameraUpVector);
  camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, aspectRatio, currentMap.cameraNearPlane, currentMap.cameraFarPlane);
  camera.viewProjection = camera.view * camera.projection;
  camera.viewProjectionInverse = inversed(camera.viewProjection);
  camera.frustum.set(camera.viewProjection);
}