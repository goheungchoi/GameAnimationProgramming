#include "ShaderStorageBuffer.h"

#include <VkBootstrap.h>

#include "Logger.h"

bool ShaderStorageBuffer::init(const VkRenderData& renderData,
                               VkShaderStorageBufferData* SSBOData,
                               size_t bufferSize) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkResult result =
      vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
                      &SSBOData->buffer, &SSBOData->bufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO via VMA (error: %i)\n",
                __FUNCTION__, result);
    return false;
  }

  SSBOData->bufferSize = bufferSize;
  Logger::log(1, "%s: created SSBO of size %i\n", __FUNCTION__, bufferSize);
  return true;
}

bool ShaderStorageBuffer::uploadSSBOData(
    const VkRenderData& renderData, VkShaderStorageBufferData* SSBOData,
    const std::vector<glm::mat4>& bufferData) {
  if (bufferData.empty()) {
    return false;
  }

  bool bufferResized = false;
  size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
  if (bufferSize > SSBOData->bufferSize) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__,
                SSBOData->buffer, SSBOData->bufferSize, bufferSize);
    cleanup(renderData, SSBOData);
    init(renderData, SSBOData, bufferSize);
    bufferResized = true;
  }

  void* data;
  VkResult result =
      vmaMapMemory(renderData.rdAllocator, SSBOData->bufferAlloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n",
                __FUNCTION__, result);
    return false;
  }
  std::memcpy(data, bufferData.data(), bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, SSBOData->bufferAlloc);
  vmaFlushAllocation(renderData.rdAllocator, SSBOData->bufferAlloc, 0,
                     SSBOData->bufferSize);

  return bufferResized;
}

bool ShaderStorageBuffer::checkForResize(const VkRenderData& renderData,
                                         VkShaderStorageBufferData* SSBOData,
                                         size_t bufferSize) {
  if (bufferSize > SSBOData->bufferSize) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__,
                SSBOData->buffer, SSBOData->bufferSize, bufferSize);
    cleanup(renderData, SSBOData);
    init(renderData, SSBOData, bufferSize);
    return true;
  }
  return false;
}

void ShaderStorageBuffer::cleanup(const VkRenderData& renderData,
                                  VkShaderStorageBufferData* SSBOData) {
  vmaDestroyBuffer(renderData.rdAllocator, SSBOData->buffer,
                   SSBOData->bufferAlloc);
}
