#include "Primus/Frame.hpp"

#include "Primus/Primus.hpp"
#include "Primus/Map.hpp"

static constexpr float cameraMoveSpeed = 2.5f;
static constexpr float cameraZoomSpeed = 0.12f;

// TODO: move camera methods into Camera struct
void Frame::updateCameraZoomLimits(const Map& currentMap)
{
  camera.nearPlane = 1.f;

  camera.zoomMin = (currentMap.maxElevationInM * currentMap.visualHeightMultiplier) + camera.nearPlane;
  const float horizontalFieldOfViewRadians = verticalToHorizontalFieldOfView(verticalFieldOfViewRadians, aspectRatio);
  const float cameraYToFitMapHorizontally = cotan(horizontalFieldOfViewRadians / 2.f) * (currentMap.widthInM / (10 * currentMap.visualHeightMultiplierInverse));
  const float cameraYToFitMapVertically = cotan(verticalFieldOfViewRadians / 2.f) * (currentMap.heightInM / (10 * currentMap.visualHeightMultiplierInverse));
  camera.zoomMax = std::min(cameraYToFitMapHorizontally, cameraYToFitMapVertically);

  const float distanceToMaxZoomedOutHorizontalEdge = camera.zoomMax / cos(horizontalFieldOfViewRadians / 2.f);
  const float distanceToMaxZoomedOutVerticalEdge = camera.zoomMax / cos(verticalFieldOfViewRadians / 2.f);
  camera.farPlane = std::max(distanceToMaxZoomedOutHorizontalEdge, distanceToMaxZoomedOutVerticalEdge); // TODO: adjust far clipping plane distance according to min elevation + current zoom level.
}
void Frame::updateCamera(const Map& currentMap, const Vec3f& currentPosition, const Vec3f& endPosition)
{
  TRACE_SCOPE();

  updateCameraZoomLimits(currentMap);

  camera.currentPosition = currentPosition;
  camera.endPosition = endPosition;

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
  camera.endPosition.y = std::clamp(camera.endPosition.y, camera.zoomMin, camera.zoomMax);
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
  const float cameraAngleInDegrees = lerp(-35.f, 0.f, (cameraZoom - camera.zoomMin) / (camera.zoomMax - camera.zoomMin));
  const Mat3f cameraRotation = Mat3f::rotationX(degreesToRadians(cameraAngleInDegrees));
  const Vec3f cameraDirection = Vec3f{ 0.f, -1.f, 0.f } *cameraRotation;
  const Vec3f cameraUpVector = Vec3f{ 0.f, 0.f, 1.f } *cameraRotation;
  camera.view = Mat4x3f::lookTo(camera.currentPosition, cameraDirection, cameraUpVector);
  camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, aspectRatio, camera.nearPlane, camera.farPlane);
  camera.viewProjection = camera.view * camera.projection;
  camera.viewProjectionInverse = inversed(camera.viewProjection);
  camera.frustum.set(camera.viewProjection);
}