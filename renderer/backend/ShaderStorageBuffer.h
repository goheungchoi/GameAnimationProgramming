#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <vector>

#include "VkRenderData.h"

class ShaderStorageBuffer {
 public:
  /* set an arbitraty buffer size as default */
  static bool init(const VkRenderData& renderData,
                   VkShaderStorageBufferData* pSSBO, size_t bufferSize = 1024);


  static bool uploadSSBOData(const VkRenderData& renderData,
                             VkShaderStorageBufferData* pSSBO,
                             const std::vector<glm::mat4>& bufferData);

  static bool uploadSSBOData(const VkRenderData& renderData,
                             VkShaderStorageBufferData* pSSBO,
                             const char* bufferData, size_t bufferSize);

  static bool checkForResize(const VkRenderData& renderData,
                             VkShaderStorageBufferData* pSSBO,
                             size_t bufferSize);

  static void cleanup(const VkRenderData& renderData,
                      VkShaderStorageBufferData* pSSBO);
};
