#include "WindowApp.h"

#include <imgui_impl_glfw.h>

#include "Camera.h"
#include "Input.h"
#include "Logger.h"
#include "VkRenderer.h"

WindowApp::WindowApp() : mRenderer{nullptr}, mCamera{nullptr} {}

WindowApp::~WindowApp() {}

bool WindowApp::init(unsigned int width, unsigned int height,
                     const std::string& title) {
  if (!glfwInit()) {
    Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);
    return false;
  }

  if (!glfwVulkanSupported()) {
    glfwTerminate();
    Logger::log(1, "%s error: Vulkan is not supported\n", __FUNCTION__);
    return false;
  }

  /* Vulkan needs no context */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (!mWindow) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
    return false;
  }
  mTitle = title;

  /* Input settings */
  mInput = std::make_unique<InputManager>();
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveForwardAction>(this),
                  GLFW_KEY_W, KeyActionType::Down);
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveBackwardAction>(this),
                  GLFW_KEY_S, KeyActionType::Down);
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveRightAction>(this),
                  GLFW_KEY_D, KeyActionType::Down);
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveLeftAction>(this),
                  GLFW_KEY_A, KeyActionType::Down);
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveDownAction>(this),
                  GLFW_KEY_Q, KeyActionType::Down);
  mInput->bindKey(Delegate<void()>::bind<&WindowApp::moveUpAction>(this),
                  GLFW_KEY_E, KeyActionType::Down);
  mInput->bindKey(
      Delegate<void()>::bind<&WindowApp::decreaseMoveSpeedAction>(this),
      GLFW_KEY_MINUS, KeyActionType::Pressed);
  mInput->bindKey(
      Delegate<void()>::bind<&WindowApp::increaseMoveSpeedAction>(this),
      GLFW_KEY_EQUAL, KeyActionType::Pressed);
  mInput->bindMouseMove(
      Delegate<void(float, float)>::bind<&WindowApp::rotateCamera>(this),
      MouseMode::Disabled);

  /* Set event callbacks */
  glfwSetWindowUserPointer(mWindow, this);
  glfwSetWindowSizeCallback(
      mWindow, [](GLFWwindow* win, int width, int height) {
        auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
        app->handleResize(width, height);
      });

  glfwSetKeyCallback(mWindow, [](GLFWwindow* win, int key, int scancode,
                                 int action, int mods) {
    auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
    app->handleKeyEvents(key, scancode, action, mods);
  });

  glfwSetMouseButtonCallback(
      mWindow, [](GLFWwindow* win, int button, int action, int mods) {
        auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
        app->handleMouseButtonEvents(button, action, mods);
      });

  glfwSetCursorPosCallback(
      mWindow, [](GLFWwindow* win, double xpos, double ypos) {
        auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
        app->handleMousePositionEvents(xpos, ypos);
      });

  /* Initialize renderer */
  mRenderer = std::make_unique<VkRenderer>(mWindow);
  if (!mRenderer->init(width, height)) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not init Vulkan\n", __FUNCTION__);
    return false;
  }

  /* Initialize camera */
  mCamera = std::make_unique<Camera>();
  mRenderer->bindCamera(mCamera);

  Logger::log(1, "%s: Window with Vulkan successfully initialized\n",
              __FUNCTION__);
  return true;
}

void WindowApp::run() {
  /* force VSYNC */
  glfwSwapInterval(1);

  auto loopStartTime = std::chrono::steady_clock::now();
  auto loopEndTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  while (!glfwWindowShouldClose(mWindow)) {
    glfwPollEvents();

    mInput->process();

    /* calculate the time we needed for the current frame, */
    /* feed it to the next update() call */
    loopEndTime = std::chrono::steady_clock::now();

    /* delta time in seconds */
    deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    loopEndTime - loopStartTime)
                    .count() /
                1'000'000.0f;
    loopStartTime = loopEndTime;

    update(deltaTime);

    if (!mRenderer->draw()) {
      break;
    }
  }
}

void WindowApp::cleanup() {
  mRenderer->cleanup();

  glfwDestroyWindow(mWindow);
  glfwTerminate();
  Logger::log(1, "%s: Terminating Window\n", __FUNCTION__);
}

void WindowApp::setModeInWindowTitle() {
  if (mRenderer->GetAppMode() == appMode::edit) {
    mWindowTitle = mTitle + " (Edit Mode)";
  } else {
    mWindowTitle = mTitle + " (View Mode)";
  }

  glfwSetWindowTitle(mWindow, mWindowTitle.c_str());
}

void WindowApp::handleResize(int width, int height) {
  if (mRenderer) mRenderer->setSize(width, height);
}

void WindowApp::handleKeyEvents(int key, int scancode, int action, int mods) {
  /* hide from application */
  if (mRenderer->GetAppMode() == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
      return;
    }
  }

  /* toggle between edit and view mode by pressing F10 */
  if (key == GLFW_KEY_F10 && action == GLFW_PRESS) {
    appMode mode = mRenderer->GetAppMode() == appMode::edit ? appMode::view
                                                            : appMode::edit;
    mRenderer->SetAppMode(mode);
    setModeInWindowTitle();
  }

  KeyEvent e{InputEventType_Keyboard};
  e.key = key;
  e.scancode = scancode;
  e.action = action;
  e.shift = mods & GLFW_MOD_SHIFT;
  e.alt = mods & GLFW_MOD_ALT;
  e.ctrl = mods & GLFW_MOD_CONTROL;
  mInput->pushKeyEvent(e);
}

void WindowApp::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  if (button >= 0 && button < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
  }

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    bMouseButtonRightPressed = !bMouseButtonRightPressed;

    if (bMouseButtonRightPressed) {
      glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      /* enable raw mode if possible */
      if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
      }
      if (mRenderer) {
        mRenderer->hideMouse(true);
      }
    } else {
      glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      if (mRenderer) {
        mRenderer->hideMouse(false);
      }
    }
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    if (mRenderer) {
      int mode = glfwGetInputMode(mWindow, GLFW_CURSOR);
      bool hidden = (mode == GLFW_CURSOR_HIDDEN);
      bool disabled = (mode == GLFW_CURSOR_DISABLED);

      mRenderer->mMousePick = !hidden && !disabled;
    }
  }
}

void WindowApp::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  MousePositionEvent e{InputEventType_MousePosition};
  e.xpos = xPos;
  e.ypos = yPos;
  int mode = glfwGetInputMode(mWindow, GLFW_CURSOR);
  e.hidden = (mode == GLFW_CURSOR_HIDDEN);
  e.disabled = (mode == GLFW_CURSOR_DISABLED);
  mInput->pushMousePositionEvent(e);

  if (mRenderer && !e.hidden && !e.disabled)
    mRenderer->mMousePos = {xPos, yPos};
}

void WindowApp::moveForwardAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveForward(1);
}

void WindowApp::moveBackwardAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveForward(-1);
}

void WindowApp::moveRightAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveRight(1);
}

void WindowApp::moveLeftAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveRight(-1);
}

void WindowApp::moveUpAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveUp(1);
}

void WindowApp::moveDownAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveUp(-1);
}

void WindowApp::increaseMoveSpeedAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveSpeed(10);
}

void WindowApp::decreaseMoveSpeedAction() {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) mCamera->addMoveSpeed(-10);
}

void WindowApp::rotateCamera(float mouseDeltaX, float mouseDeltaY) {
  if (!mCamera) return;

  if (bMouseButtonRightPressed) {
    mCamera->addViewAzimuth(mouseDeltaX / 10.0f);
    mCamera->addViewElevation(-mouseDeltaY / 10.0f);
  }
}

void WindowApp::update(float deltaTime) {
  /* update camera */
  mCamera->updateCamera(deltaTime);

  mRenderer->updateAnimations(deltaTime);
}
