/* Vulkan uniform buffer object */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class UniformBuffer {
 public:
  static bool init(const VkRenderData& renderData,
                   VkUniformBufferData* uboData);
  static void uploadData(const VkRenderData& renderData,
                         VkUniformBufferData* uboData,
                         const VkUploadMatrices& matrices);
  static void cleanup(const VkRenderData& renderData,
                      VkUniformBufferData* uboData);
};
