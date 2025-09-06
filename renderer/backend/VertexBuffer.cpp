#include "VertexBuffer.h"

#include <cstring>

#include "CommandBuffer.h"
#include "Logger.h"

bool VertexBuffer::init(const VkRenderData& renderData,
                        VkVertexBufferData* vertexBufferData,
                        VkDeviceSize bufferSize) {
  /* vertex buffer */
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo bufferAllocInfo{};
  bufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &bufferInfo,
                                    &bufferAllocInfo, &vertexBufferData->buffer,
                                    &vertexBufferData->alloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(
        1, "%s error: could not allocate vertex buffer via VMA (error: %i)\n",
        __FUNCTION__, result);
    return false;
  }

  /* staging buffer for copy */
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = bufferSize;
  ;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  result = vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfo,
                           &stagingAllocInfo, &vertexBufferData->staging,
                           &vertexBufferData->stagingAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1,
                "%s error: could not allocate vertex staging buffer via VMA "
                "(error: %i)\n",
                __FUNCTION__, result);
    return false;
  }
  vertexBufferData->size = bufferSize;
  return true;
}

bool VertexBuffer::uploadData(const VkRenderData& renderData,
                              VkVertexBufferData* vertexBufferData,
                              const VkMesh& vertexData) {
  unsigned int vertexDataSize = vertexData.vertices.size() * sizeof(VkVertex);

  /* buffer too small, resize */
  if (vertexBufferData->size < vertexDataSize) {
    cleanup(renderData, vertexBufferData);

    if (!init(renderData, vertexBufferData, vertexDataSize)) {
      Logger::log(1,
                  "%s error: could not create vertex buffer of size %i bytes\n",
                  __FUNCTION__, vertexDataSize);
      return false;
    }
    Logger::log(1, "%s: vertex buffer resize to %i bytes\n", __FUNCTION__,
                vertexDataSize);
    vertexBufferData->size = vertexDataSize;
  }

  /* copy data to staging buffer */
  void *data;
  VkResult result = vmaMapMemory(renderData.rdAllocator,
                                 vertexBufferData->stagingAlloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map memory (error: %i)\n", __FUNCTION__,
                result);
    return false;
  }
  std::memcpy(data, vertexData.vertices.data(), vertexDataSize);
  vmaUnmapMemory(renderData.rdAllocator, vertexBufferData->stagingAlloc);
  vmaFlushAllocation(renderData.rdAllocator, vertexBufferData->stagingAlloc, 0,
                     vertexDataSize);

  VkBufferMemoryBarrier vertexBufferBarrier{};
  vertexBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  vertexBufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  vertexBufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  vertexBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.buffer = vertexBufferData->staging;
  vertexBufferBarrier.offset = 0;
  vertexBufferBarrier.size = vertexBufferData->size;

  VkBufferCopy stagingBufferCopy{};
  stagingBufferCopy.srcOffset = 0;
  stagingBufferCopy.dstOffset = 0;
  stagingBufferCopy.size = vertexBufferData->size;

  /* trigger data transfer via command buffer */
  VkCommandBuffer commandBuffer =
      CommandBuffer::createTransientBuffer(renderData);

  vkCmdCopyBuffer(commandBuffer, vertexBufferData->staging,
                  vertexBufferData->buffer, 1, &stagingBufferCopy);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1,
                       &vertexBufferBarrier, 0, nullptr);

  if (!CommandBuffer::submitTransientBuffer(renderData, commandBuffer)) {
    return false;
  }

  return true;
}

bool VertexBuffer::uploadData(const VkRenderData& renderData,
                              VkVertexBufferData* vertexBufferData,
                              const std::vector<glm::vec3>& vertexData) {
  unsigned int vertexDataSize = vertexData.size() * sizeof(glm::vec3);

  /* buffer too small, resize */
  if (vertexBufferData->size < vertexDataSize) {
    cleanup(renderData, vertexBufferData);

    if (!init(renderData, vertexBufferData, vertexDataSize)) {
      Logger::log(1,
                  "%s error: could not create vertex buffer of size %i bytes\n",
                  __FUNCTION__, vertexDataSize);
      return false;
    }
    Logger::log(1, "%s: vertex buffer resize to %i bytes\n", __FUNCTION__,
                vertexDataSize);
    vertexBufferData->size = vertexDataSize;
  }

  /* copy data to staging buffer */
  void *data;
  VkResult result = vmaMapMemory(renderData.rdAllocator,
                                 vertexBufferData->stagingAlloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map memory (error: %i)\n", __FUNCTION__,
                result);
    return false;
  }
  std::memcpy(data, vertexData.data(), vertexDataSize);
  vmaUnmapMemory(renderData.rdAllocator, vertexBufferData->stagingAlloc);
  vmaFlushAllocation(renderData.rdAllocator, vertexBufferData->stagingAlloc, 0,
                     vertexDataSize);

  VkBufferMemoryBarrier vertexBufferBarrier{};
  vertexBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  vertexBufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  vertexBufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  vertexBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.buffer = vertexBufferData->staging;
  vertexBufferBarrier.offset = 0;
  vertexBufferBarrier.size = vertexBufferData->size;

  VkBufferCopy stagingBufferCopy{};
  stagingBufferCopy.srcOffset = 0;
  stagingBufferCopy.dstOffset = 0;
  stagingBufferCopy.size = vertexBufferData->size;

  /* trigger data transfer via command buffer */
  VkCommandBuffer commandBuffer =
      CommandBuffer::createTransientBuffer(renderData);

  vkCmdCopyBuffer(commandBuffer, vertexBufferData->staging,
                  vertexBufferData->buffer, 1, &stagingBufferCopy);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1,
                       &vertexBufferBarrier, 0, nullptr);

  if (!CommandBuffer::submitTransientBuffer(renderData, commandBuffer)) {
    return false;
  }

  return true;
}

void VertexBuffer::cleanup(const VkRenderData& renderData,
                           VkVertexBufferData* vertexBufferData) {
  vmaDestroyBuffer(renderData.rdAllocator, vertexBufferData->staging,
                   vertexBufferData->stagingAlloc);
  vmaDestroyBuffer(renderData.rdAllocator, vertexBufferData->buffer,
                   vertexBufferData->alloc);
}
