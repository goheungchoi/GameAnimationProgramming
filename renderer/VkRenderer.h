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
  Timer mUpdateAnimationTimer{};
  Timer mUploadToSSBOTimer{};
  Timer mUploadToUBOTimer{};
  Timer mUIGenerateTimer{};
  Timer mUIDrawTimer{};

	std::shared_ptr<Camera> mCamera{nullptr};
 public:
  VkRenderer(GLFWwindow* window);

  bool init(unsigned int width, unsigned int height);
  bool setSize(unsigned int width, unsigned int height);

	void bindCamera(std::shared_ptr<Camera> camera);
  void hideMouse(bool bHide);

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

	void centerInstance(std::shared_ptr<AssimpInstance> instance);

  void updateAnimations(float deltaTime);

	void cleanup();

 private:
  bool bHideMouse{};
  UserInterface mUserInterface{};

  VkPushConstants mModelData{};
  VkComputePushConstants mComputeModelData{};
  VkUniformBufferData mPerspectiveViewMatrixUBO{};

	/* color hightlight for selection etc */
  std::vector<glm::vec2> mSelectedInstance{};
  VkShaderStorageBufferData mSelectedInstanceBuffer{};

	/* for animated and non-animated models */
  std::vector<glm::mat4> mWorldPosMatrices{};
  VkShaderStorageBufferData mShaderModelRootMatrixBuffer{};

  /* for animated models */
  VkShaderStorageBufferData mShaderBoneMatrixBuffer{};
  
	/* for compute shader */
  bool mHasDedicatedComputeQueue = false;
  std::vector<NodeTransformData> mNodeTransformData{};
  VkShaderStorageBufferData mShaderTRSMatrixBuffer{};
  VkShaderStorageBufferData mShaderNodeTransformBuffer{};
  
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

	void assignInstanceIndices();

	void updateComputeDescriptorSets();
  void runComputeShaders(std::shared_ptr<AssimpModel> model,
                          int numInstances, uint32_t modelOffset);
};
