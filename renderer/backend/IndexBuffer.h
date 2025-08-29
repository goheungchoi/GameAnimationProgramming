#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class IndexBuffer {
public:
	static bool init(const VkRenderData& renderData, VkIndexBufferData* bufferData,
		size_t bufferSize);
	static bool uploadData(const VkRenderData& renderData, VkIndexBufferData& bufferData,
		VkMesh vertexData);

	static void cleanup(const VkRenderData& renderData, VkIndexBufferData* bufferData);
};
