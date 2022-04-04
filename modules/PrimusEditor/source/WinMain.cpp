#define NOMINMAX

#include "Core/Core.hpp"
#include "Core/Task.hpp"
#include "Core/Image.hpp"
#include "Core/WindowsPlatform.h"

#include "D3D11/D3D11.hpp"

#include "ImGui/ImGui.hpp"

#include "Primus/Primus.hpp"
#include "Primus/D3D11Renderer.hpp"

#include <vector>
#include <memory>
#include <fstream>

#include "Internet.hpp"
#include "Heightmap.hpp"
#include "Colormap.hpp"

MainWindow window;

Input input;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(
  HWND   windowHandle,
  UINT   message,
  WPARAM wParam,
  LPARAM lParam
)
{
  if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
  {
    return 0;
  }

  switch (message) {
    case WM_SIZE: {
      int newClientAreaWidth = LOWORD(lParam);
      int newClientAreaHeight = HIWORD(lParam);
      return 0;
    }
    case WM_CLOSE: {
      PostQuitMessage(0);
      return DefWindowProc(windowHandle, message, wParam, lParam);
    }
    default:
    {
      if (input.processMessage(windowHandle, message, wParam, lParam))
      {
        return 0;
      }
      else
      {
        return DefWindowProc(windowHandle, message, wParam, lParam);
      }
    }
  }
}

struct DownloadPopup
{
  ImGuiID id = 0;

  int tileRangeTopLeft[2] = { 1, 1 };
  int tileRangeBottomRight[2] = { 1, 1 };
  int tileRangeZoom = 1;
  int heightmapZoom = 1;
  int colormapZoom = 1;

  void open()
  {
    ImGui::OpenPopup(id);
  }

  void define()
  {
    if (ImGui::BeginPopupModal("Download", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::InputInt("Zoom", &tileRangeZoom, 1, 2);
      ImGui::InputInt2("Top left", tileRangeTopLeft);
      ImGui::InputInt2("Bottom right", tileRangeBottomRight);

      ImGui::Separator();

      ImGui::InputInt("Heightmap zoom", &heightmapZoom, 1, 2);
      ImGui::InputInt("Colormap zoom", &colormapZoom, 1, 2);

      ImGui::Separator();

      if (ImGui::Button("Download"))
      {
        // TODO: download
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

} downloadPopup;

static std::vector<Map> maps;

static Vec2i viewportSize = { 1920, 1080 }; // TODO: get the true viewport size.

enum class NodeType
{
  None = 0,
  Map,
  Heightmap,
  Colormap
};
static NodeType selectedNodeType = NodeType::None;
static const void* selectNodeContext = nullptr;

static void defineTreeLeaf(NodeType nodeType, const void* nodeContext, const void* id, const char* name)
{
  ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  if (selectedNodeType == nodeType && selectNodeContext == nodeContext)
  {
    treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
  }
  ImGui::TreeNodeEx(id, treeNodeFlags, name);
  if (ImGui::IsItemClicked())
  {
    selectedNodeType = nodeType;
    selectNodeContext = nodeContext;
  }
}

static void defineGui()
{
  downloadPopup.id = ImGui::GetID("Download");

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Map"))
    {
      if (ImGui::MenuItem("Open"))
      {
        wchar_t mapFolderPath[MAX_PATH];
        if (tryChooseFolderDialog(window, L"Choose map folder", mapFolderPath))
        {
          Map openedMap;
          if (!openedMap.tryLoad(mapFolderPath, verticalFieldOfViewRadians, viewportSize.x / float(viewportSize.y)))
          {
            logError("Failed to load %ls map.", mapFolderPath);
          }
          else
          {
            maps.emplace_back(std::move(openedMap));
          }
        }
      }
      if (ImGui::MenuItem("Download"))
      {
        downloadPopup.open();
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  {
    // DockSpace
    static constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    static ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    {
      ImGui::Begin("Maps");

      for (const Map& map : maps)
      {
        char mapNameUtf8[1024];
        if (WideCharToMultiByte(CP_UTF8, 0, map.directoryPath, -1, mapNameUtf8, sizeof(mapNameUtf8), NULL, NULL) == 0)
        {
          continue;
        }
        if (ImGui::TreeNode(mapNameUtf8))
        {
          defineTreeLeaf(NodeType::Heightmap, &map.heightmap, (void*)(intptr_t)(map.directoryPath + 1), "heightmap");

          defineTreeLeaf(NodeType::Colormap, &map.heightmap, (void*)(intptr_t)(map.directoryPath + 2), "colormap"); // TODO: use some EditorMap class that has colormap thats different form the game class.

          ImGui::TreePop();
        }
      }

      ImGui::End();
    }

    {
      ImGui::Begin("Viewport");

      ImGui::End();
    }

    {
      ImGui::Begin("Inspector");

      switch (selectedNodeType)
      {
        case NodeType::Map:
          ImGui::Text("Map");
          break;
        case NodeType::Heightmap:
          ImGui::Text("Heightmap");
          break;
        case NodeType::Colormap:
          ImGui::Text("Colormap");
          break;
        default:
          break;
      }

      ImGui::End();
    }

    {
      ImGui::Begin("Log");

      ImGui::End();
    }

    ImGui::ShowDemoWindow();

    downloadPopup.define();

    ImGui::End(); // DockSpace.
  }
}

int WINAPI WinMain(
  HINSTANCE instanceHandle,
  HINSTANCE hPrevInstance, // always zero
  LPSTR commandLine,
  int showCode
)
{
  taskScheduler.initialize();

  if (!window.tryInitializeEditorStyle(instanceHandle, L"PrimusEditor", &WindowProc))
  {
    return -1;
  }

  D3D11Renderer renderer;
  if (!renderer.tryInitialize(window))
  {
    window.showErrorMessageBox(L"Failed to initialize D3D11Renderer.", L"Fatal Error");
    return -2;
  }

  Dar::ImGui imGui{ window, D3D11::device, D3D11::context };  

  ImGuiIO& imGuiIo = ImGui::GetIO();
  imGuiIo.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\LeelawUI.ttf", 16);
  imGuiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  window.show();

  runGameLoop([&](int64 frameIndex, float timeDelta) {
    imGui.newFrame();

    defineGui();

    renderer.beginRender();

    renderer.setBackBufferRenderTarget();
    imGui.render();

    renderer.endRender();
  });

  return 0;
}

//int main(int argc, char* argv[])
//{
//  taskScheduler.initialize();
//
//  InternetHandle internet{ InternetOpen(L"Primus MapTool", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0) };
//  if (!internet)
//  {
//    logError("InternetOpen failed.");
//    return 1;
//  }
//
//  //downloadHeightmap(internet.get(), "heightmap.png");
//  downloadColormap(internet.get(), "colormap.png");
//
//  return 0;
//}