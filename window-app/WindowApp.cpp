#include "WindowApp.h"

#include "VkRenderer.h"

#include "Logger.h"

bool WindowApp::init(unsigned int width, unsigned int height, const std::string& title)
{
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

void WindowApp::handleResize(int width, int height)
{
}

void WindowApp::handleKeyEvents(int key, int scancode, int action, int mods)
{
}

void WindowApp::handleMouseButtonEvents(int button, int action, int mods)
{
}

void WindowApp::handleMousePositionEvents(double xPos, double yPos)
{
}
