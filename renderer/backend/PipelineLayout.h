#pragma once

#include "VkRenderData.h"

class PipelineLayout {
 public:
  static bool init(const VkRenderData& renderData,
                   VkPipelineLayout* pipelineLayout,
                   const std::vector<VkDescriptorSetLayout>& layouts,
                   const std::vector<VkPushConstantRange>& pushConstants = {});

  static void cleanup(const VkRenderData& renderData,
                      VkPipelineLayout pipelineLayout);
};
