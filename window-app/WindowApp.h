#pragma once

#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>
#include <string>

class WindowApp {
  GLFWwindow* mWindow{nullptr};
  std::unique_ptr<class VkRenderer> mRenderer{nullptr};
  std::unique_ptr<class Camera> mCamera{nullptr};

  bool bMouseButtonRightPressed{false};
  int mCurrMouseXPos{0};
  int mCurrMouseYPos{0};

 public:
  bool init(unsigned int width, unsigned int height, const std::string& title);
  void run();
  void cleanup();

 private:
  void handleResize(int width, int height);
  void handleKeyEvents(int key, int scancode, int action, int mods);
  void handleMouseButtonEvents(int button, int action, int mods);
  void handleMousePositionEvents(double xPos, double yPos);

  void update(float deltaTime);
};
