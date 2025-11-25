#include "Framebuffer.h"

#include "CommandBuffer.h"
#include "Logger.h"

bool Framebuffer::init(VkRenderData* renderData) {
  renderData->rdSwapchainImages =
      renderData->rdVkbSwapchain.get_images().value();
  renderData->rdSwapchainImageViews =
      renderData->rdVkbSwapchain.get_image_views().value();

  renderData->rdFramebuffers.resize(renderData->rdSwapchainImageViews.size());

  for (unsigned int i = 0; i < renderData->rdSwapchainImageViews.size(); ++i) {
    std::vector<VkImageView> attachments = {
        renderData->rdSwapchainImageViews.at(i),
        renderData->rdSelectionImageView, renderData->rdDepthImageView};

    VkFramebufferCreateInfo FboInfo{};
    FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FboInfo.renderPass = renderData->rdRenderpass;
    FboInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    FboInfo.pAttachments = attachments.data();
    FboInfo.width = renderData->rdVkbSwapchain.extent.width;
    FboInfo.height = renderData->rdVkbSwapchain.extent.height;
    FboInfo.layers = 1;

    VkResult result =
        vkCreateFramebuffer(renderData->rdVkbDevice.device, &FboInfo, nullptr,
                            &renderData->rdFramebuffers.at(i));
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to create framebuffer %i (error: %i)\n",
                  __FUNCTION__, i, result);
      return false;
    }
  }
  return true;
}

int Framebuffer::getPixelValueFromSelectionImage(const VkRenderData& renderData,
                                                 glm::uvec2 pos) {
  VkCommandBuffer readbackCmd = CommandBuffer::createTransientBuffer(
      renderData, renderData.rdCommandPool);

  VkImageSubresourceRange layoutTransferRange{};
  layoutTransferRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  layoutTransferRange.baseMipLevel = 0;
  layoutTransferRange.levelCount = 1;
  layoutTransferRange.baseArrayLayer = 0;
  layoutTransferRange.layerCount = 1;

  /* transition destination (local) image to transfer destination layout
   */
  VkImageMemoryBarrier layoutTransferBarrier{};
  layoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  layoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  layoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  layoutTransferBarrier.image = renderData.rdLocalSelectionImage;
  layoutTransferBarrier.subresourceRange = layoutTransferRange;
  layoutTransferBarrier.srcAccessMask = 0;
  layoutTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  /* transition source (selection) image to transfer source optimal layout
   */
  VkImageMemoryBarrier srcLayoutTransferBarrier{};
  srcLayoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  srcLayoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  srcLayoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  srcLayoutTransferBarrier.image = renderData.rdSelectionImage;
  srcLayoutTransferBarrier.subresourceRange = layoutTransferRange;
  srcLayoutTransferBarrier.srcAccessMask = 0;
  srcLayoutTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  /* copy selection image to local image */
  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = renderData.rdVkbSwapchain.extent.width;
  imageCopyRegion.extent.height = renderData.rdVkbSwapchain.extent.height;
  imageCopyRegion.extent.depth = 1;

  /* transition destination (local) image to general layout to allow
   * mapping */
  VkImageMemoryBarrier destLayoutTransferBarrier{};
  destLayoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  destLayoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  destLayoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  destLayoutTransferBarrier.image = renderData.rdLocalSelectionImage;
  destLayoutTransferBarrier.subresourceRange = layoutTransferRange;
  destLayoutTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  destLayoutTransferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

  vkCmdPipelineBarrier(readbackCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &layoutTransferBarrier);
  vkCmdPipelineBarrier(readbackCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &srcLayoutTransferBarrier);
  vkCmdCopyImage(readbackCmd, renderData.rdSelectionImage,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 renderData.rdLocalSelectionImage,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
  vkCmdPipelineBarrier(readbackCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &destLayoutTransferBarrier);

  bool commandResult = CommandBuffer::submitTransientBuffer(
      renderData, renderData.rdCommandPool, readbackCmd,
      renderData.rdGraphicsQueue);

  if (!commandResult) {
    Logger::log(1, "%s error: could not submit readback transfer commands\n",
                __FUNCTION__);
    return -1;
  }

  /* get image layout */
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout{};

  vkGetImageSubresourceLayout(renderData.rdVkbDevice.device,
                              renderData.rdLocalSelectionImage, &subResource,
                              &subResourceLayout);

  const int* pData;
  VkResult result =
      vmaMapMemory(renderData.rdAllocator,
                   renderData.rdLocalSelectionImageAlloc, (void**)&pData);
  if (result != VK_SUCCESS) {
    Logger::log(1,
                "%s error: could not map readback image memory (error: %i)\n",
                __FUNCTION__, result);
    return -1;
  }

  pData += pos.y * subResourceLayout.rowPitch / sizeof(int) + pos.x;

  int pixelData = *pData;

  vmaUnmapMemory(renderData.rdAllocator, renderData.rdLocalSelectionImageAlloc);

  return pixelData;
}

void Framebuffer::cleanup(VkRenderData* renderData) {
  for (auto& fb : renderData->rdFramebuffers) {
    vkDestroyFramebuffer(renderData->rdVkbDevice.device, fb, nullptr);
  }
}
