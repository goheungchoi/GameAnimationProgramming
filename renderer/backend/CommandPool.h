#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class CommandPool {
 public:
  static bool init(const VkRenderData& renderData, vkb::QueueType queueType,
                   VkCommandPool* pool);
  static void cleanup(const VkRenderData& renderData, VkCommandPool pool);
};
