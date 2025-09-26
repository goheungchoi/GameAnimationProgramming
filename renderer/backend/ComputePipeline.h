/* Vulkan compute pipeline with shaders */
#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include "VkRenderData.h"

class ComputePipeline {
 public:
  static bool init(const VkRenderData& renderData,
                   VkPipelineLayout pipelineLayout, VkPipeline* pipeline,
                   const std::string& computeShaderFilename);
  static void cleanup(const VkRenderData& renderData, VkPipeline pipeline);
};
