#include "Primus/Game.hpp"

#include "Primus/Map.hpp"

static constexpr float verticalFieldOfViewRadians = degreesToRadians(74.f);
static constexpr float nearPlane = 1.f;
static constexpr float farPlane = 5000.f;
static constexpr float cameraSpeed = 1.f;
static constexpr float zoomMin = 5.f;
static constexpr float zoomMax = 20.f;

Map currentMap;

bool Game::tryInitialize(Frame& firstFrame)
{
  return currentMap.tryLoad(L"italy");
}

void Game::update(const Frame& lastFrame, Frame& nextFrame)
{
  const Vec3f cameraPositionDelta = Vec3f
  { cameraSpeed * nextFrame.input.keyboard.d.pressedDown - cameraSpeed * nextFrame.input.keyboard.a.pressedDown , 
    -nextFrame.input.mouse.dWheel, 
    cameraSpeed * nextFrame.input.keyboard.w.pressedDown - cameraSpeed * nextFrame.input.keyboard.s.pressedDown };
  nextFrame.camera.position = lastFrame.camera.position + cameraPositionDelta;
  nextFrame.camera.position.y = std::clamp(nextFrame.camera.position.y, zoomMin, zoomMax);

  nextFrame.camera.view = Mat4x3f::lookTo(nextFrame.camera.position, Vec3f{ 0.f, -1.f, 0.f }, Vec3f{ 0.f, 0.f, 1.f });
  const float aspectRatio = static_cast<float>(lastFrame.clientAreaWidth) / lastFrame.clientAreaHeight;
  nextFrame.camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, aspectRatio, nearPlane, farPlane);
  nextFrame.camera.viewProjection = nextFrame.camera.view * nextFrame.camera.projection;
}