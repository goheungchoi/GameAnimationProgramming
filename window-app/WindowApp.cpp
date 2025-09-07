#include "WindowApp.h"

#include "VkRenderer.h"

#include "Logger.h"
#include "Camera.h"

#include <imgui_impl_glfw.h>

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
	
	/* Set event callbacks */
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* win, int width, int height) {
		auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
		app->handleResize(width, height);
		}
	);

	glfwSetKeyCallback(mWindow, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
		auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
		app->handleKeyEvents(key, scancode, action, mods);
		}
	);

	glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* win, int button, int action, int mods) {
		auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
		app->handleMouseButtonEvents(button, action, mods);
		}
	);

	glfwSetCursorPosCallback(mWindow, [](GLFWwindow* win, double xpos, double ypos) {
		auto app = static_cast<WindowApp*>(glfwGetWindowUserPointer(win));
		app->handleMousePositionEvents(xpos, ypos);
		}
	);

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

	Logger::log(1, "%s: Window with Vulkan successfully initialized\n", __FUNCTION__);
	return true;
}

void WindowApp::run()
{
	/* force VSYNC */
	glfwSwapInterval(1);

	auto loopStartTime = std::chrono::steady_clock::now();
	auto loopEndTime = std::chrono::steady_clock::now();
	float deltaTime = 0.0f;

	while (!glfwWindowShouldClose(mWindow)) {
		glfwPollEvents();

		/* calculate the time we needed for the current frame, */
		/* feed it to the next update() call */
		loopEndTime = std::chrono::steady_clock::now();

		/* delta time in seconds */
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(loopEndTime - loopStartTime).count() / 1'000'000.0f;
		loopStartTime = loopEndTime;

		update(deltaTime);

		if (!mRenderer->draw()) {
			break;
		}
	}
}

void WindowApp::cleanup()
{
	mRenderer->cleanup();

	glfwDestroyWindow(mWindow);
	glfwTerminate();
	Logger::log(1, "%s: Terminating Window\n", __FUNCTION__);
}

void WindowApp::handleResize(int width, int height) {
  if (mRenderer) 
		mRenderer->setSize(width, height);
}

void WindowApp::handleKeyEvents(int key, int scancode, int action, int mods)
{
  /* hide from application */
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if (mCamera) {
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveForward(1);
    }
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveForward(-1);
    }
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveRight(-1);
    }
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveRight(1);
    }
    if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveUp(1);
    }
    if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveUp(-1);
    }

    if (key == GLFW_KEY_MINUS &&
        (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveSpeed(-10);
    }
    if (key == GLFW_KEY_EQUAL &&
        (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      mCamera->addMoveSpeed(10);
    }
  }
}

void WindowApp::handleMouseButtonEvents(int button, int action, int mods)
{
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
        glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION,
                         GLFW_TRUE);
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
}

void WindowApp::handleMousePositionEvents(double xPos, double yPos)
{
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  /* calculate relative movement from last position */
  int mouseMoveRelX = static_cast<int>(xPos) - mCurrMouseXPos;
  int mouseMoveRelY = static_cast<int>(yPos) - mCurrMouseYPos;

  if (bMouseButtonRightPressed) {
    if (mCamera) {
      mCamera->addViewAzimuth(mouseMoveRelX / 10.0f);
      mCamera->addViewElevation(mouseMoveRelY / 10.0f);
		}
  }

  /* save old values */
  mCurrMouseXPos = static_cast<int>(xPos);
  mCurrMouseYPos = static_cast<int>(yPos);
}

void WindowApp::update(float deltaTime) { 
	/* update camera */
	mCamera->updateCamera(deltaTime);

	mRenderer->updateAnimations(deltaTime);
}
