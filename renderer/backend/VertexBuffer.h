/* Vulkan uniform buffer object */
#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <vector>

#include "VkRenderData.h"

class VertexBuffer {
 public:
  static bool init(const VkRenderData& renderData,
                   VkVertexBufferData* vertexBufferData,
                   VkDeviceSize bufferSize);

  static bool uploadData(const VkRenderData& renderData,
                         VkVertexBufferData* vertexBufferData,
                         const VkMesh& vertexData);
  static bool uploadData(const VkRenderData& renderData,
                         VkVertexBufferData* vertexBufferData,
                         const std::vector<glm::vec3>& vetrexData);

  static void cleanup(const VkRenderData& renderData,
                      VkVertexBufferData* vertexBufferData);
};
