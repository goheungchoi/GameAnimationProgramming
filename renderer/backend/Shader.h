#pragma once

#include <vulkan/vulkan.h>

#include <string>

class Shader {
 public:
  static VkShaderModule loadShader(VkDevice device,
                                   const std::string& shaderFileName);
  static void cleanup(VkDevice device, VkShaderModule module);
};
