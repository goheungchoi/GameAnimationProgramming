// Vulkan Renderer
#pragma once

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "Timer.h"
#include "Camera.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "UserInterface.h"
#include "ShaderStorageBuffer.h"

#include "ModelAndInstanceData.h"
#include "VkRenderData.h"

class VkRenderer {
 public:
  VkRenderer(GLFWwindow* window);

  bool init(unsigned int width, unsigned int height);
  bool setSize(unsigned int width, unsigned int height);

  bool draw();

 private:
  VkRenderData mRenderData{};
  ModelAndInstanceData mModelInstData{};

  Timer mFrameTimer{};
  Timer mMatrixGenerateTimer{};
  Timer mUploadToUBOTimer{};
  Timer mUIGenerateTimer{};
  Timer mUIDrawTimer{};


};
