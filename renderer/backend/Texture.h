#pragma once

#include <assimp/texture.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

#include "VkRenderData.h"

struct VkTextureStagingBuffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation alloc = VK_NULL_HANDLE;
};

class Texture {
 public:
  static bool loadTexture(const VkRenderData& renderData,
                          VkTextureData* texData,
                          const std::string& textureFilename,
                          bool generateMipmaps = true, bool flipImage = false);
  static bool loadTexture(const VkRenderData& renderData,
                          VkTextureData* texData,
                          const std::string& textureName, aiTexel* textureData,
                          int width, int height, bool generateMipmaps = true,
                          bool flipImage = false);

  static void cleanup(const VkRenderData& renderData, VkTextureData* texData);

 private:
  static bool uploadToGPU(const VkRenderData& renderData,
                          VkTextureData* texData,
                          VkTextureStagingBuffer* stagingData, uint32_t width,
                          uint32_t height, bool generateMipmaps,
                          uint32_t mipmapLevels);
};
