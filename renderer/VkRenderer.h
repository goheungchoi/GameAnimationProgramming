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
  VkRenderData mRenderData{};
  ModelAndInstanceData mModelInstData{};

  Timer mFrameTimer{};
  Timer mMatrixGenerateTimer{};
  Timer mUploadToUBOTimer{};
  Timer mUIGenerateTimer{};
  Timer mUIDrawTimer{};

	Camera* mCam{};
 public:
  VkRenderer(GLFWwindow* window);

  bool init(unsigned int width, unsigned int height);
  bool setSize(unsigned int width, unsigned int height);

	bool bindCamera(Camera* cam);

  bool draw();

  bool hasModel(std::string modelFileName);
  std::shared_ptr<AssimpModel> getModel(std::string modelFileName);
  bool addModel(std::string modelFileName);
  void deleteModel(std::string modelFileName);

  std::shared_ptr<AssimpInstance> addInstance(
      std::shared_ptr<AssimpModel> model);
  void addInstances(std::shared_ptr<AssimpModel> model, int numInstances);
  void deleteInstance(std::shared_ptr<AssimpInstance> instance);
  void cloneInstance(std::shared_ptr<AssimpInstance> instance);

 private:
  UserInterface mUserInterface{};
  Camera mCamera{};

  VkPushConstants mModelData{};
  VkUniformBufferData mPerspectiveViewMatrixUBO{};

  /* For non-animated models */
  std::vector<glm::mat4> mWorldPosMatrices{};
  VkShaderStorageBufferData mWorldPosBuffer{};

  /* For animated models */
  std::vector<glm::mat4> mModelBoneMatrices{};
  VkShaderStorageBufferData mBoneMatrixBuffer{};
  
  /* Identity matrices */
  VkUploadMatrices mMatrices{glm::mat4(1.0f), glm::mat4(1.0f)};

  /* Vulkan specific code */
  VkSurfaceKHR mSurface = VK_NULL_HANDLE;
  VmaAllocator rdAllocator = nullptr;

  VkDeviceSize mMinSSBOOffsetAlignment = 0;

  bool deviceInit();
  bool getQueues();
  bool initVma();
  bool createDescriptorPool();
  bool createDescriptorLayouts();
  bool createDescriptorSets();
  bool updateDescriptorSets();
  bool createDepthBuffer();
  bool createMatrixUBO();
  bool createSSBOs();
  bool createSwapchain();
  bool createRenderPass();
  bool createPipelineLayouts();
  bool createPipelines();
  bool createFramebuffer();
  bool createCommandPool();
  bool createCommandBuffer();
  bool createSyncObjects();

  bool initUserInterface();

  bool recreateSwapchain();

	void updateTriangleCount();
};
