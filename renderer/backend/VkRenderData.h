#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <assimp/material.h>

struct NodeTransformData {
	glm::vec4 translation{0.f};
	glm::vec4 scale{1.f};
	glm::vec4 rotation{1.f, 0.f, 0.f, 0.f};
};

struct VkVertex {
	glm::vec4 position{};
	glm::vec4 color{1.f};
	glm::vec4 normal{};
	glm::uvec4 boneNum{};
	glm::vec4 boneWeights{};
};

struct VkMesh {
	std::vector<VkVertex> vertices{};
	std::vector<uint32_t> indices{};
	std::unordered_map<aiTextureType, std::string> textures{};
	bool usesPBRColors = false;
};

struct VkUploadMatrices {
	glm::mat4 view{};
	glm::mat4 proj{};
};

struct VkTextureData {
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VmaAllocation alloc = VK_NULL_HANDLE;

	VkDescriptorSet descSet = VK_NULL_HANDLE;
};

struct VkVertexBufferData {
	VkDeviceSize size = 0;
	void* data = nullptr;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation alloc = VK_NULL_HANDLE;
	VkBuffer staging = VK_NULL_HANDLE;
	VmaAllocation stagingAlloc = VK_NULL_HANDLE;
};

struct VkIndexBufferData {
	VkDeviceSize size = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation alloc = nullptr;
	VkBuffer staging = VK_NULL_HANDLE;
	VmaAllocation stagingAlloc = nullptr;
};

struct VkUniformBufferData {
	VkDeviceSize size = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation alloc = VK_NULL_HANDLE;

	VkDescriptorSet descSet = VK_NULL_HANDLE;
};

struct VkShaderStorageBufferData {
	VkDeviceSize size = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation alloc = VK_NULL_HANDLE;

	VkDescriptorSet descSet = VK_NULL_HANDLE;
};

struct VkPushConstants {
	uint32_t pkModelStride;
	uint32_t pkWorldPosOffset;
	uint32_t pkSkinMatOffset;
};

struct VkComputePushConstants {
	uint32_t pkModelOffset;
};

struct VkRenderData {
	GLFWwindow* rdWindow = nullptr;

	int rdWidth = 0;
	int rdHeight = 0;

	size_t rdTriangleCount = 0;
	size_t rdMatricesSize = 0;

	int rdFOV = 60;

	float rdFrameTime = 0.0f;
	float rdUpdateAnimationTime = 0.0f;
	float rdUploadToSSBOTime = 0.0f;
	float rdUploadToVBOTime = 0.0f;
	float rdUploadToUBOTime = 0.0f;
	float rdUIGenerateTime = 0.0f;
	float rdUIDrawTime = 0.0f;

	bool rdHighlightSelectedInstance = false;
	float rdSelectedInstanceHighlightValue = 1.0f;

	/* Vulkan specific stuff */
	VmaAllocator rdAllocator = nullptr;

	vkb::Instance rdVkbInstance{};
	vkb::PhysicalDevice rdVkbPhysicalDevice{};
	vkb::Device rdVkbDevice{};
	vkb::Swapchain rdVkbSwapchain{};

	std::vector<VkImage> rdSwapchainImages{};
	std::vector<VkImageView> rdSwapchainImageViews{};
	std::vector<VkFramebuffer> rdFramebuffers{};
	std::vector<VkFramebuffer> rdSelectionFramebuffers{};

	VkQueue rdGraphicsQueue = VK_NULL_HANDLE;
	VkQueue rdPresentQueue = VK_NULL_HANDLE;
	VkQueue rdComputeQueue = VK_NULL_HANDLE;

	VkImage rdDepthImage = VK_NULL_HANDLE;
	VkImageView rdDepthImageView = VK_NULL_HANDLE;
	VkFormat rdDepthFormat = VK_FORMAT_UNDEFINED;
	VmaAllocation rdDepthImageAlloc = VK_NULL_HANDLE;

	VkImage rdSelectionImage = VK_NULL_HANDLE;
	VkImageView rdSelectionImageView = VK_NULL_HANDLE;
	VkFormat rdSelectionFormat = VK_FORMAT_UNDEFINED;
	VmaAllocation rdSelectionImageAlloc = VK_NULL_HANDLE;

	VkImage rdLocalSelectionImage = VK_NULL_HANDLE;
  VkImageView rdLocalSelectionImageView = VK_NULL_HANDLE;
  VkFormat rdLocalSelectionFormat = VK_FORMAT_UNDEFINED;
  VmaAllocation rdLocalSelectionImageAlloc = VK_NULL_HANDLE;

	VkRenderPass rdRenderpass = VK_NULL_HANDLE;
	VkRenderPass rdSelectionRenderpass = VK_NULL_HANDLE;

	VkPipelineLayout rdAssimpPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout rdAssimpSkinningPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout rdAssimpComputeTransformaPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout rdAssimpComputeMatrixMultPipelineLayout = VK_NULL_HANDLE;

	VkPipeline rdAssimpPipeline = VK_NULL_HANDLE;
	VkPipeline rdAssimpSkinningPipeline = VK_NULL_HANDLE;
	VkPipeline rdAssimpComputeTransformPipeline = VK_NULL_HANDLE;
	VkPipeline rdAssimpComputeMatrixMultPipeline = VK_NULL_HANDLE;

	VkCommandPool rdCommandPool = VK_NULL_HANDLE;
	VkCommandPool rdComputeCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer rdCommandBuffer = VK_NULL_HANDLE;
	VkCommandBuffer rdComputeCommandBuffer = VK_NULL_HANDLE;

	VkSemaphore rdPresentSemaphore = VK_NULL_HANDLE;
	VkSemaphore rdRenderSemaphore = VK_NULL_HANDLE;
	VkSemaphore rdGraphicSemaphore = VK_NULL_HANDLE;
	VkSemaphore rdComputeSemaphore = VK_NULL_HANDLE;
	VkFence rdRenderFence = VK_NULL_HANDLE;
	VkFence rdComputeFence = VK_NULL_HANDLE;

	VkDescriptorSetLayout rdAssimpDescriptorLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout rdAssimpSkinningDescriptorLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout rdAssimpTextureDescriptorLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout rdAssimpComputeTransformDescriptorLayout =
			VK_NULL_HANDLE;
	VkDescriptorSetLayout rdAssimpComputeMatrixMultDescriptorLayout =
			VK_NULL_HANDLE;
	VkDescriptorSetLayout rdAssimpComputeMatrixMultPerModelDescriptorLayout =
			VK_NULL_HANDLE;

	VkDescriptorSet rdAssimpDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet rdAssimpSkinningDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet rdAssimpComputeTransformDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet rdAssimpComputeMatrixMultDescriptorSet = VK_NULL_HANDLE;

	VkDescriptorPool rdDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorPool rdImguiDescriptorPool = VK_NULL_HANDLE;
};
