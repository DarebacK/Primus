#define NOMINMAX

#include "Core/Core.hpp"
#include "Core/Image.hpp"

#include <vector>
#include <memory>
#include <fstream>

#include "Internet.hpp"
#include "Heightmap.hpp"

int main(int argc, char* argv[])
{
  InternetHandle internet = InternetOpen(L"Primus MapTool", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
  if (!internet)
  {
    logError("InternetOpen failed.");
    return 1;
  }

  downloadHeightmap(internet, "heightmap.png");

  return 0;
}