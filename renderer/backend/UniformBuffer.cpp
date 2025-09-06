#include "UniformBuffer.h"

#include <VkBootstrap.h>

#include "Logger.h"

bool UniformBuffer::init(const VkRenderData& renderData,
                         VkUniformBufferData* uboData) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(VkUploadMatrices);
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkResult result =
      vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
                      &uboData->buffer, &uboData->alloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(
        1, "%s error: could not allocate uniform buffer via VMA (error: %i)\n",
        __FUNCTION__, result);
    return false;
  }

  return true;
}

void UniformBuffer::uploadData(const VkRenderData& renderData,
                               VkUniformBufferData* uboData,
                               const VkUploadMatrices& matrices) {
  void* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, uboData->alloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1,
                "%s error: could not map uniform buffer memory (error: %i)\n",
                __FUNCTION__, result);
    return;
  }
  std::memcpy(data, &matrices, sizeof(VkUploadMatrices));
  vmaUnmapMemory(renderData.rdAllocator, uboData->alloc);
  vmaFlushAllocation(renderData.rdAllocator, uboData->alloc, 0, uboData->size);
}

void UniformBuffer::cleanup(const VkRenderData& renderData,
                            VkUniformBufferData* uboData) {
  vmaDestroyBuffer(renderData.rdAllocator, uboData->buffer, uboData->alloc);
}
