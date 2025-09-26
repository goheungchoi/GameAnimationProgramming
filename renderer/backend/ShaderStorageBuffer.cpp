#include "ShaderStorageBuffer.h"

#include <VkBootstrap.h>

#include "Logger.h"

bool ShaderStorageBuffer::init(const VkRenderData& renderData,
                               VkShaderStorageBufferData* pSSBO,
                               size_t bufferSize) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkResult result =
      vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
                      &pSSBO->buffer, &pSSBO->alloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO via VMA (error: %i)\n",
                __FUNCTION__, result);
    return false;
  }

  pSSBO->size = bufferSize;
  Logger::log(1, "%s: created SSBO of size %i\n", __FUNCTION__, bufferSize);
  return true;
}

bool ShaderStorageBuffer::uploadSSBOData(
    const VkRenderData& renderData, VkShaderStorageBufferData* pSSBO,
    const std::vector<glm::mat4>& bufferData) {
  if (bufferData.empty()) {
    return false;
  }

  bool bufferResized = false;
  size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
  if (bufferSize > pSSBO->size) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__,
                pSSBO->buffer, pSSBO->size, bufferSize);
    cleanup(renderData, pSSBO);
    init(renderData, pSSBO, bufferSize);
    bufferResized = true;
  }

  void* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, pSSBO->alloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n",
                __FUNCTION__, result);
    return false;
  }
  std::memcpy(data, bufferData.data(), bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, pSSBO->alloc);
  vmaFlushAllocation(renderData.rdAllocator, pSSBO->alloc, 0, pSSBO->size);

  return bufferResized;
}

bool ShaderStorageBuffer::uploadSSBOData(const VkRenderData& renderData,
                                         VkShaderStorageBufferData* pSSBO,
                                         const char* bufferData,
                                         size_t bufferSize) {
  if (bufferSize == 0) {
    return false;
  }

  bool bufferResized = false;
  if (bufferSize > pSSBO->size) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__,
                pSSBO->buffer, pSSBO->size, bufferSize);
    cleanup(renderData, pSSBO);
    init(renderData, pSSBO, bufferSize);
    bufferResized = true;
  }

  void* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, pSSBO->alloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n",
                __FUNCTION__, result);
    return false;
  }
  std::memcpy(data, bufferData, bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, pSSBO->alloc);
  vmaFlushAllocation(renderData.rdAllocator, pSSBO->alloc, 0, pSSBO->size);

  return bufferResized;
}

bool ShaderStorageBuffer::checkForResize(const VkRenderData& renderData,
                                         VkShaderStorageBufferData* pSSBO,
                                         size_t bufferSize) {
  if (bufferSize > pSSBO->size) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__,
                pSSBO->buffer, pSSBO->size, bufferSize);
    cleanup(renderData, pSSBO);
    init(renderData, pSSBO, bufferSize);
    return true;
  }
  return false;
}

void ShaderStorageBuffer::cleanup(const VkRenderData& renderData,
                                  VkShaderStorageBufferData* pSSBO) {
  vmaDestroyBuffer(renderData.rdAllocator, pSSBO->buffer, pSSBO->alloc);
}
