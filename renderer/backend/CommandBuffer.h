#pragma once

#include <vulkan/vulkan.h>
#include <VkRenderData.h>

class CommandBuffer {
public:
	static bool init(const VkRenderData& renderData, VkCommandBuffer* cmd);

	static bool reset(VkCommandBuffer cmd, VkCommandBufferResetFlags flags = 0);
	static bool begin(VkCommandBuffer cmd, VkCommandBufferBeginInfo& beginInfo);
	static bool beginTransient(VkCommandBuffer cmd);
	static bool end(VkCommandBuffer cmd);

	static VkCommandBuffer createTransientBuffer(const VkRenderData& renderData);
	static bool submitTransientBuffer(const VkRenderData& renderData, VkCommandBuffer& cmd);

	static void cleanup(const VkRenderData& renderData, VkCommandBuffer* cmd);
};
