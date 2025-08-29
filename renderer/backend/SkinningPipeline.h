#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include "VkRenderData.h"

class SkinningPipeline {
 public:
  static bool init(const VkRenderData& renderData,
                   VkPipelineLayout pipelineLayout, VkPipeline* pipeline,
                   const std::string& vertexShaderFilename,
                   const std::string& fragmentShaderFilename);
  static void cleanup(const VkRenderData& renderData, VkPipeline pipeline);
};
