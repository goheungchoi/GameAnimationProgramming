#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VkRenderData.h"

class Framebuffer {
 public:
  static bool init(VkRenderData* renderData);
  static int getPixelValueFromSelectionImage(const VkRenderData& renderData,
                                             glm::uvec2 pos);
  static void cleanup(VkRenderData* renderData);
};
