#include "Primus/Game.hpp"

static constexpr float verticalFieldOfViewRadians = degreesToRadians(74.f);
static constexpr float nearPlane = 1.f;
static constexpr float farPlane = 5000.f;

void Game::update(const Frame& lastFrame, Frame& nextFrame)
{
  nextFrame.camera.position = Vec3f{ 0.f, 10.f, 0.f };
  nextFrame.camera.view = Mat4x3f::lookTo(nextFrame.camera.position, Vec3f{ 0.f, -1.f, 0.f }, Vec3f{ 0.f, 0.f, 1.f });
  const float aspectRatio = static_cast<float>(lastFrame.clientAreaWidth) / lastFrame.clientAreaHeight;
  nextFrame.camera.projection = Mat4f::perspectiveProjectionD3d(verticalFieldOfViewRadians, aspectRatio, nearPlane, farPlane);
  nextFrame.camera.viewProjection = nextFrame.camera.view * nextFrame.camera.projection;
}