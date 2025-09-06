#include <memory>
#include <string>

#include "Logger.h"
#include "WindowApp.h"

int main(int argc, char *argv[]) {
  WindowApp app{};

  if (!app.init(1280, 720, "Vulkan Renderer - Open Asset Import Library")) {
    Logger::log(1, "%s error: Window init error\n", __FUNCTION__);
    return -1;
  }
  app.run();
  app.cleanup();

  return 0;
}
