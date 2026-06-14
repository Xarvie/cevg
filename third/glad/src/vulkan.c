/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/vulkan.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif




static GLADapiproc glad_vulkan_internal_loader_get_proc(const char *name);
static GLADloadfunc glad_global_on_demand_vulkan_loader_func = glad_vulkan_internal_loader_get_proc;

void gladSetVulkanOnDemandLoader(GLADloadfunc loader) {
    glad_global_on_demand_vulkan_loader_func = loader;
}

static GLADapiproc glad_vk_on_demand_loader(const char *name) {
    GLADapiproc result = NULL;
    if (result == NULL && glad_global_on_demand_vulkan_loader_func != NULL) {
        result = glad_global_on_demand_vulkan_loader_func(name);
    }
    /* this provokes a segmentation fault if there was no loader or no loader returned something useful */
    return result;
}


static VkResult GLAD_API_PTR glad_on_demand_impl_vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR * pAcquireInfo, uint32_t * pImageIndex) {
    glad_vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR) glad_vk_on_demand_loader("vkAcquireNextImage2KHR");
    return glad_vkAcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
}
PFN_vkAcquireNextImage2KHR glad_vkAcquireNextImage2KHR = glad_on_demand_impl_vkAcquireNextImage2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t * pImageIndex) {
    glad_vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) glad_vk_on_demand_loader("vkAcquireNextImageKHR");
    return glad_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}
PFN_vkAcquireNextImageKHR glad_vkAcquireNextImageKHR = glad_on_demand_impl_vkAcquireNextImageKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo * pAllocateInfo, VkCommandBuffer * pCommandBuffers) {
    glad_vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers) glad_vk_on_demand_loader("vkAllocateCommandBuffers");
    return glad_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}
PFN_vkAllocateCommandBuffers glad_vkAllocateCommandBuffers = glad_on_demand_impl_vkAllocateCommandBuffers;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo * pAllocateInfo, VkDescriptorSet * pDescriptorSets) {
    glad_vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets) glad_vk_on_demand_loader("vkAllocateDescriptorSets");
    return glad_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}
PFN_vkAllocateDescriptorSets glad_vkAllocateDescriptorSets = glad_on_demand_impl_vkAllocateDescriptorSets;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo * pAllocateInfo, const VkAllocationCallbacks * pAllocator, VkDeviceMemory * pMemory) {
    glad_vkAllocateMemory = (PFN_vkAllocateMemory) glad_vk_on_demand_loader("vkAllocateMemory");
    return glad_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}
PFN_vkAllocateMemory glad_vkAllocateMemory = glad_on_demand_impl_vkAllocateMemory;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo * pBeginInfo) {
    glad_vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer) glad_vk_on_demand_loader("vkBeginCommandBuffer");
    return glad_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}
PFN_vkBeginCommandBuffer glad_vkBeginCommandBuffer = glad_on_demand_impl_vkBeginCommandBuffer;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    glad_vkBindBufferMemory = (PFN_vkBindBufferMemory) glad_vk_on_demand_loader("vkBindBufferMemory");
    return glad_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}
PFN_vkBindBufferMemory glad_vkBindBufferMemory = glad_on_demand_impl_vkBindBufferMemory;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo * pBindInfos) {
    glad_vkBindBufferMemory2 = (PFN_vkBindBufferMemory2) glad_vk_on_demand_loader("vkBindBufferMemory2");
    return glad_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}
PFN_vkBindBufferMemory2 glad_vkBindBufferMemory2 = glad_on_demand_impl_vkBindBufferMemory2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo * pBindInfos) {
    glad_vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR) glad_vk_on_demand_loader("vkBindBufferMemory2KHR");
    return glad_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
}
PFN_vkBindBufferMemory2KHR glad_vkBindBufferMemory2KHR = glad_on_demand_impl_vkBindBufferMemory2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    glad_vkBindImageMemory = (PFN_vkBindImageMemory) glad_vk_on_demand_loader("vkBindImageMemory");
    return glad_vkBindImageMemory(device, image, memory, memoryOffset);
}
PFN_vkBindImageMemory glad_vkBindImageMemory = glad_on_demand_impl_vkBindImageMemory;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo * pBindInfos) {
    glad_vkBindImageMemory2 = (PFN_vkBindImageMemory2) glad_vk_on_demand_loader("vkBindImageMemory2");
    return glad_vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}
PFN_vkBindImageMemory2 glad_vkBindImageMemory2 = glad_on_demand_impl_vkBindImageMemory2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo * pBindInfos) {
    glad_vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR) glad_vk_on_demand_loader("vkBindImageMemory2KHR");
    return glad_vkBindImageMemory2KHR(device, bindInfoCount, pBindInfos);
}
PFN_vkBindImageMemory2KHR glad_vkBindImageMemory2KHR = glad_on_demand_impl_vkBindImageMemory2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    glad_vkCmdBeginQuery = (PFN_vkCmdBeginQuery) glad_vk_on_demand_loader("vkCmdBeginQuery");
    glad_vkCmdBeginQuery(commandBuffer, queryPool, query, flags);
}
PFN_vkCmdBeginQuery glad_vkCmdBeginQuery = glad_on_demand_impl_vkCmdBeginQuery;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, VkSubpassContents contents) {
    glad_vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) glad_vk_on_demand_loader("vkCmdBeginRenderPass");
    glad_vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}
PFN_vkCmdBeginRenderPass glad_vkCmdBeginRenderPass = glad_on_demand_impl_vkCmdBeginRenderPass;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, const VkSubpassBeginInfo * pSubpassBeginInfo) {
    glad_vkCmdBeginRenderPass2 = (PFN_vkCmdBeginRenderPass2) glad_vk_on_demand_loader("vkCmdBeginRenderPass2");
    glad_vkCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}
PFN_vkCmdBeginRenderPass2 glad_vkCmdBeginRenderPass2 = glad_on_demand_impl_vkCmdBeginRenderPass2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, const VkSubpassBeginInfo * pSubpassBeginInfo) {
    glad_vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR) glad_vk_on_demand_loader("vkCmdBeginRenderPass2KHR");
    glad_vkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}
PFN_vkCmdBeginRenderPass2KHR glad_vkCmdBeginRenderPass2KHR = glad_on_demand_impl_vkCmdBeginRenderPass2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo * pRenderingInfo) {
    glad_vkCmdBeginRendering = (PFN_vkCmdBeginRendering) glad_vk_on_demand_loader("vkCmdBeginRendering");
    glad_vkCmdBeginRendering(commandBuffer, pRenderingInfo);
}
PFN_vkCmdBeginRendering glad_vkCmdBeginRendering = glad_on_demand_impl_vkCmdBeginRendering;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo * pRenderingInfo) {
    glad_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR) glad_vk_on_demand_loader("vkCmdBeginRenderingKHR");
    glad_vkCmdBeginRenderingKHR(commandBuffer, pRenderingInfo);
}
PFN_vkCmdBeginRenderingKHR glad_vkCmdBeginRenderingKHR = glad_on_demand_impl_vkCmdBeginRenderingKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT(VkCommandBuffer commandBuffer, const VkBindDescriptorBufferEmbeddedSamplersInfoEXT * pBindDescriptorBufferEmbeddedSamplersInfo) {
    glad_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT = (PFN_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT) glad_vk_on_demand_loader("vkCmdBindDescriptorBufferEmbeddedSamplers2EXT");
    glad_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT(commandBuffer, pBindDescriptorBufferEmbeddedSamplersInfo);
}
PFN_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT glad_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT = glad_on_demand_impl_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet * pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t * pDynamicOffsets) {
    glad_vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets) glad_vk_on_demand_loader("vkCmdBindDescriptorSets");
    glad_vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}
PFN_vkCmdBindDescriptorSets glad_vkCmdBindDescriptorSets = glad_on_demand_impl_vkCmdBindDescriptorSets;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindDescriptorSets2KHR(VkCommandBuffer commandBuffer, const VkBindDescriptorSetsInfo * pBindDescriptorSetsInfo) {
    glad_vkCmdBindDescriptorSets2KHR = (PFN_vkCmdBindDescriptorSets2KHR) glad_vk_on_demand_loader("vkCmdBindDescriptorSets2KHR");
    glad_vkCmdBindDescriptorSets2KHR(commandBuffer, pBindDescriptorSetsInfo);
}
PFN_vkCmdBindDescriptorSets2KHR glad_vkCmdBindDescriptorSets2KHR = glad_on_demand_impl_vkCmdBindDescriptorSets2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    glad_vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer) glad_vk_on_demand_loader("vkCmdBindIndexBuffer");
    glad_vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}
PFN_vkCmdBindIndexBuffer glad_vkCmdBindIndexBuffer = glad_on_demand_impl_vkCmdBindIndexBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindIndexBuffer2KHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkIndexType indexType) {
    glad_vkCmdBindIndexBuffer2KHR = (PFN_vkCmdBindIndexBuffer2KHR) glad_vk_on_demand_loader("vkCmdBindIndexBuffer2KHR");
    glad_vkCmdBindIndexBuffer2KHR(commandBuffer, buffer, offset, size, indexType);
}
PFN_vkCmdBindIndexBuffer2KHR glad_vkCmdBindIndexBuffer2KHR = glad_on_demand_impl_vkCmdBindIndexBuffer2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    glad_vkCmdBindPipeline = (PFN_vkCmdBindPipeline) glad_vk_on_demand_loader("vkCmdBindPipeline");
    glad_vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}
PFN_vkCmdBindPipeline glad_vkCmdBindPipeline = glad_on_demand_impl_vkCmdBindPipeline;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets) {
    glad_vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers) glad_vk_on_demand_loader("vkCmdBindVertexBuffers");
    glad_vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}
PFN_vkCmdBindVertexBuffers glad_vkCmdBindVertexBuffers = glad_on_demand_impl_vkCmdBindVertexBuffers;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindVertexBuffers2(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets, const VkDeviceSize * pSizes, const VkDeviceSize * pStrides) {
    glad_vkCmdBindVertexBuffers2 = (PFN_vkCmdBindVertexBuffers2) glad_vk_on_demand_loader("vkCmdBindVertexBuffers2");
    glad_vkCmdBindVertexBuffers2(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}
PFN_vkCmdBindVertexBuffers2 glad_vkCmdBindVertexBuffers2 = glad_on_demand_impl_vkCmdBindVertexBuffers2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets, const VkDeviceSize * pSizes, const VkDeviceSize * pStrides) {
    glad_vkCmdBindVertexBuffers2EXT = (PFN_vkCmdBindVertexBuffers2EXT) glad_vk_on_demand_loader("vkCmdBindVertexBuffers2EXT");
    glad_vkCmdBindVertexBuffers2EXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}
PFN_vkCmdBindVertexBuffers2EXT glad_vkCmdBindVertexBuffers2EXT = glad_on_demand_impl_vkCmdBindVertexBuffers2EXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit * pRegions, VkFilter filter) {
    glad_vkCmdBlitImage = (PFN_vkCmdBlitImage) glad_vk_on_demand_loader("vkCmdBlitImage");
    glad_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}
PFN_vkCmdBlitImage glad_vkCmdBlitImage = glad_on_demand_impl_vkCmdBlitImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 * pBlitImageInfo) {
    glad_vkCmdBlitImage2 = (PFN_vkCmdBlitImage2) glad_vk_on_demand_loader("vkCmdBlitImage2");
    glad_vkCmdBlitImage2(commandBuffer, pBlitImageInfo);
}
PFN_vkCmdBlitImage2 glad_vkCmdBlitImage2 = glad_on_demand_impl_vkCmdBlitImage2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 * pBlitImageInfo) {
    glad_vkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR) glad_vk_on_demand_loader("vkCmdBlitImage2KHR");
    glad_vkCmdBlitImage2KHR(commandBuffer, pBlitImageInfo);
}
PFN_vkCmdBlitImage2KHR glad_vkCmdBlitImage2KHR = glad_on_demand_impl_vkCmdBlitImage2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment * pAttachments, uint32_t rectCount, const VkClearRect * pRects) {
    glad_vkCmdClearAttachments = (PFN_vkCmdClearAttachments) glad_vk_on_demand_loader("vkCmdClearAttachments");
    glad_vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}
PFN_vkCmdClearAttachments glad_vkCmdClearAttachments = glad_on_demand_impl_vkCmdClearAttachments;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue * pColor, uint32_t rangeCount, const VkImageSubresourceRange * pRanges) {
    glad_vkCmdClearColorImage = (PFN_vkCmdClearColorImage) glad_vk_on_demand_loader("vkCmdClearColorImage");
    glad_vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}
PFN_vkCmdClearColorImage glad_vkCmdClearColorImage = glad_on_demand_impl_vkCmdClearColorImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue * pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange * pRanges) {
    glad_vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage) glad_vk_on_demand_loader("vkCmdClearDepthStencilImage");
    glad_vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}
PFN_vkCmdClearDepthStencilImage glad_vkCmdClearDepthStencilImage = glad_on_demand_impl_vkCmdClearDepthStencilImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy * pRegions) {
    glad_vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer) glad_vk_on_demand_loader("vkCmdCopyBuffer");
    glad_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}
PFN_vkCmdCopyBuffer glad_vkCmdCopyBuffer = glad_on_demand_impl_vkCmdCopyBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 * pCopyBufferInfo) {
    glad_vkCmdCopyBuffer2 = (PFN_vkCmdCopyBuffer2) glad_vk_on_demand_loader("vkCmdCopyBuffer2");
    glad_vkCmdCopyBuffer2(commandBuffer, pCopyBufferInfo);
}
PFN_vkCmdCopyBuffer2 glad_vkCmdCopyBuffer2 = glad_on_demand_impl_vkCmdCopyBuffer2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 * pCopyBufferInfo) {
    glad_vkCmdCopyBuffer2KHR = (PFN_vkCmdCopyBuffer2KHR) glad_vk_on_demand_loader("vkCmdCopyBuffer2KHR");
    glad_vkCmdCopyBuffer2KHR(commandBuffer, pCopyBufferInfo);
}
PFN_vkCmdCopyBuffer2KHR glad_vkCmdCopyBuffer2KHR = glad_on_demand_impl_vkCmdCopyBuffer2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy * pRegions) {
    glad_vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage) glad_vk_on_demand_loader("vkCmdCopyBufferToImage");
    glad_vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}
PFN_vkCmdCopyBufferToImage glad_vkCmdCopyBufferToImage = glad_on_demand_impl_vkCmdCopyBufferToImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBufferToImage2(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 * pCopyBufferToImageInfo) {
    glad_vkCmdCopyBufferToImage2 = (PFN_vkCmdCopyBufferToImage2) glad_vk_on_demand_loader("vkCmdCopyBufferToImage2");
    glad_vkCmdCopyBufferToImage2(commandBuffer, pCopyBufferToImageInfo);
}
PFN_vkCmdCopyBufferToImage2 glad_vkCmdCopyBufferToImage2 = glad_on_demand_impl_vkCmdCopyBufferToImage2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 * pCopyBufferToImageInfo) {
    glad_vkCmdCopyBufferToImage2KHR = (PFN_vkCmdCopyBufferToImage2KHR) glad_vk_on_demand_loader("vkCmdCopyBufferToImage2KHR");
    glad_vkCmdCopyBufferToImage2KHR(commandBuffer, pCopyBufferToImageInfo);
}
PFN_vkCmdCopyBufferToImage2KHR glad_vkCmdCopyBufferToImage2KHR = glad_on_demand_impl_vkCmdCopyBufferToImage2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy * pRegions) {
    glad_vkCmdCopyImage = (PFN_vkCmdCopyImage) glad_vk_on_demand_loader("vkCmdCopyImage");
    glad_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}
PFN_vkCmdCopyImage glad_vkCmdCopyImage = glad_on_demand_impl_vkCmdCopyImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImage2(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 * pCopyImageInfo) {
    glad_vkCmdCopyImage2 = (PFN_vkCmdCopyImage2) glad_vk_on_demand_loader("vkCmdCopyImage2");
    glad_vkCmdCopyImage2(commandBuffer, pCopyImageInfo);
}
PFN_vkCmdCopyImage2 glad_vkCmdCopyImage2 = glad_on_demand_impl_vkCmdCopyImage2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 * pCopyImageInfo) {
    glad_vkCmdCopyImage2KHR = (PFN_vkCmdCopyImage2KHR) glad_vk_on_demand_loader("vkCmdCopyImage2KHR");
    glad_vkCmdCopyImage2KHR(commandBuffer, pCopyImageInfo);
}
PFN_vkCmdCopyImage2KHR glad_vkCmdCopyImage2KHR = glad_on_demand_impl_vkCmdCopyImage2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy * pRegions) {
    glad_vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer) glad_vk_on_demand_loader("vkCmdCopyImageToBuffer");
    glad_vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}
PFN_vkCmdCopyImageToBuffer glad_vkCmdCopyImageToBuffer = glad_on_demand_impl_vkCmdCopyImageToBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImageToBuffer2(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 * pCopyImageToBufferInfo) {
    glad_vkCmdCopyImageToBuffer2 = (PFN_vkCmdCopyImageToBuffer2) glad_vk_on_demand_loader("vkCmdCopyImageToBuffer2");
    glad_vkCmdCopyImageToBuffer2(commandBuffer, pCopyImageToBufferInfo);
}
PFN_vkCmdCopyImageToBuffer2 glad_vkCmdCopyImageToBuffer2 = glad_on_demand_impl_vkCmdCopyImageToBuffer2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 * pCopyImageToBufferInfo) {
    glad_vkCmdCopyImageToBuffer2KHR = (PFN_vkCmdCopyImageToBuffer2KHR) glad_vk_on_demand_loader("vkCmdCopyImageToBuffer2KHR");
    glad_vkCmdCopyImageToBuffer2KHR(commandBuffer, pCopyImageToBufferInfo);
}
PFN_vkCmdCopyImageToBuffer2KHR glad_vkCmdCopyImageToBuffer2KHR = glad_on_demand_impl_vkCmdCopyImageToBuffer2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    glad_vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults) glad_vk_on_demand_loader("vkCmdCopyQueryPoolResults");
    glad_vkCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}
PFN_vkCmdCopyQueryPoolResults glad_vkCmdCopyQueryPoolResults = glad_on_demand_impl_vkCmdCopyQueryPoolResults;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    glad_vkCmdDispatch = (PFN_vkCmdDispatch) glad_vk_on_demand_loader("vkCmdDispatch");
    glad_vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}
PFN_vkCmdDispatch glad_vkCmdDispatch = glad_on_demand_impl_vkCmdDispatch;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    glad_vkCmdDispatchBase = (PFN_vkCmdDispatchBase) glad_vk_on_demand_loader("vkCmdDispatchBase");
    glad_vkCmdDispatchBase(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}
PFN_vkCmdDispatchBase glad_vkCmdDispatchBase = glad_on_demand_impl_vkCmdDispatchBase;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    glad_vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR) glad_vk_on_demand_loader("vkCmdDispatchBaseKHR");
    glad_vkCmdDispatchBaseKHR(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}
PFN_vkCmdDispatchBaseKHR glad_vkCmdDispatchBaseKHR = glad_on_demand_impl_vkCmdDispatchBaseKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    glad_vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect) glad_vk_on_demand_loader("vkCmdDispatchIndirect");
    glad_vkCmdDispatchIndirect(commandBuffer, buffer, offset);
}
PFN_vkCmdDispatchIndirect glad_vkCmdDispatchIndirect = glad_on_demand_impl_vkCmdDispatchIndirect;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    glad_vkCmdDraw = (PFN_vkCmdDraw) glad_vk_on_demand_loader("vkCmdDraw");
    glad_vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
PFN_vkCmdDraw glad_vkCmdDraw = glad_on_demand_impl_vkCmdDraw;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    glad_vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed) glad_vk_on_demand_loader("vkCmdDrawIndexed");
    glad_vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
PFN_vkCmdDrawIndexed glad_vkCmdDrawIndexed = glad_on_demand_impl_vkCmdDrawIndexed;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    glad_vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect) glad_vk_on_demand_loader("vkCmdDrawIndexedIndirect");
    glad_vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}
PFN_vkCmdDrawIndexedIndirect glad_vkCmdDrawIndexedIndirect = glad_on_demand_impl_vkCmdDrawIndexedIndirect;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    glad_vkCmdDrawIndexedIndirectCount = (PFN_vkCmdDrawIndexedIndirectCount) glad_vk_on_demand_loader("vkCmdDrawIndexedIndirectCount");
    glad_vkCmdDrawIndexedIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}
PFN_vkCmdDrawIndexedIndirectCount glad_vkCmdDrawIndexedIndirectCount = glad_on_demand_impl_vkCmdDrawIndexedIndirectCount;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    glad_vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR) glad_vk_on_demand_loader("vkCmdDrawIndexedIndirectCountKHR");
    glad_vkCmdDrawIndexedIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}
PFN_vkCmdDrawIndexedIndirectCountKHR glad_vkCmdDrawIndexedIndirectCountKHR = glad_on_demand_impl_vkCmdDrawIndexedIndirectCountKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    glad_vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect) glad_vk_on_demand_loader("vkCmdDrawIndirect");
    glad_vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}
PFN_vkCmdDrawIndirect glad_vkCmdDrawIndirect = glad_on_demand_impl_vkCmdDrawIndirect;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    glad_vkCmdDrawIndirectCount = (PFN_vkCmdDrawIndirectCount) glad_vk_on_demand_loader("vkCmdDrawIndirectCount");
    glad_vkCmdDrawIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}
PFN_vkCmdDrawIndirectCount glad_vkCmdDrawIndirectCount = glad_on_demand_impl_vkCmdDrawIndirectCount;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    glad_vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR) glad_vk_on_demand_loader("vkCmdDrawIndirectCountKHR");
    glad_vkCmdDrawIndirectCountKHR(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}
PFN_vkCmdDrawIndirectCountKHR glad_vkCmdDrawIndirectCountKHR = glad_on_demand_impl_vkCmdDrawIndirectCountKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
    glad_vkCmdEndQuery = (PFN_vkCmdEndQuery) glad_vk_on_demand_loader("vkCmdEndQuery");
    glad_vkCmdEndQuery(commandBuffer, queryPool, query);
}
PFN_vkCmdEndQuery glad_vkCmdEndQuery = glad_on_demand_impl_vkCmdEndQuery;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    glad_vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass) glad_vk_on_demand_loader("vkCmdEndRenderPass");
    glad_vkCmdEndRenderPass(commandBuffer);
}
PFN_vkCmdEndRenderPass glad_vkCmdEndRenderPass = glad_on_demand_impl_vkCmdEndRenderPass;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo * pSubpassEndInfo) {
    glad_vkCmdEndRenderPass2 = (PFN_vkCmdEndRenderPass2) glad_vk_on_demand_loader("vkCmdEndRenderPass2");
    glad_vkCmdEndRenderPass2(commandBuffer, pSubpassEndInfo);
}
PFN_vkCmdEndRenderPass2 glad_vkCmdEndRenderPass2 = glad_on_demand_impl_vkCmdEndRenderPass2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo * pSubpassEndInfo) {
    glad_vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR) glad_vk_on_demand_loader("vkCmdEndRenderPass2KHR");
    glad_vkCmdEndRenderPass2KHR(commandBuffer, pSubpassEndInfo);
}
PFN_vkCmdEndRenderPass2KHR glad_vkCmdEndRenderPass2KHR = glad_on_demand_impl_vkCmdEndRenderPass2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndRendering(VkCommandBuffer commandBuffer) {
    glad_vkCmdEndRendering = (PFN_vkCmdEndRendering) glad_vk_on_demand_loader("vkCmdEndRendering");
    glad_vkCmdEndRendering(commandBuffer);
}
PFN_vkCmdEndRendering glad_vkCmdEndRendering = glad_on_demand_impl_vkCmdEndRendering;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdEndRenderingKHR(VkCommandBuffer commandBuffer) {
    glad_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR) glad_vk_on_demand_loader("vkCmdEndRenderingKHR");
    glad_vkCmdEndRenderingKHR(commandBuffer);
}
PFN_vkCmdEndRenderingKHR glad_vkCmdEndRenderingKHR = glad_on_demand_impl_vkCmdEndRenderingKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers) {
    glad_vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands) glad_vk_on_demand_loader("vkCmdExecuteCommands");
    glad_vkCmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}
PFN_vkCmdExecuteCommands glad_vkCmdExecuteCommands = glad_on_demand_impl_vkCmdExecuteCommands;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
    glad_vkCmdFillBuffer = (PFN_vkCmdFillBuffer) glad_vk_on_demand_loader("vkCmdFillBuffer");
    glad_vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}
PFN_vkCmdFillBuffer glad_vkCmdFillBuffer = glad_on_demand_impl_vkCmdFillBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    glad_vkCmdNextSubpass = (PFN_vkCmdNextSubpass) glad_vk_on_demand_loader("vkCmdNextSubpass");
    glad_vkCmdNextSubpass(commandBuffer, contents);
}
PFN_vkCmdNextSubpass glad_vkCmdNextSubpass = glad_on_demand_impl_vkCmdNextSubpass;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo * pSubpassBeginInfo, const VkSubpassEndInfo * pSubpassEndInfo) {
    glad_vkCmdNextSubpass2 = (PFN_vkCmdNextSubpass2) glad_vk_on_demand_loader("vkCmdNextSubpass2");
    glad_vkCmdNextSubpass2(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}
PFN_vkCmdNextSubpass2 glad_vkCmdNextSubpass2 = glad_on_demand_impl_vkCmdNextSubpass2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo * pSubpassBeginInfo, const VkSubpassEndInfo * pSubpassEndInfo) {
    glad_vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR) glad_vk_on_demand_loader("vkCmdNextSubpass2KHR");
    glad_vkCmdNextSubpass2KHR(commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}
PFN_vkCmdNextSubpass2KHR glad_vkCmdNextSubpass2KHR = glad_on_demand_impl_vkCmdNextSubpass2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers) {
    glad_vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier) glad_vk_on_demand_loader("vkCmdPipelineBarrier");
    glad_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}
PFN_vkCmdPipelineBarrier glad_vkCmdPipelineBarrier = glad_on_demand_impl_vkCmdPipelineBarrier;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo * pDependencyInfo) {
    glad_vkCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2) glad_vk_on_demand_loader("vkCmdPipelineBarrier2");
    glad_vkCmdPipelineBarrier2(commandBuffer, pDependencyInfo);
}
PFN_vkCmdPipelineBarrier2 glad_vkCmdPipelineBarrier2 = glad_on_demand_impl_vkCmdPipelineBarrier2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfo * pDependencyInfo) {
    glad_vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR) glad_vk_on_demand_loader("vkCmdPipelineBarrier2KHR");
    glad_vkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo);
}
PFN_vkCmdPipelineBarrier2KHR glad_vkCmdPipelineBarrier2KHR = glad_on_demand_impl_vkCmdPipelineBarrier2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void * pValues) {
    glad_vkCmdPushConstants = (PFN_vkCmdPushConstants) glad_vk_on_demand_loader("vkCmdPushConstants");
    glad_vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}
PFN_vkCmdPushConstants glad_vkCmdPushConstants = glad_on_demand_impl_vkCmdPushConstants;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushConstants2KHR(VkCommandBuffer commandBuffer, const VkPushConstantsInfo * pPushConstantsInfo) {
    glad_vkCmdPushConstants2KHR = (PFN_vkCmdPushConstants2KHR) glad_vk_on_demand_loader("vkCmdPushConstants2KHR");
    glad_vkCmdPushConstants2KHR(commandBuffer, pPushConstantsInfo);
}
PFN_vkCmdPushConstants2KHR glad_vkCmdPushConstants2KHR = glad_on_demand_impl_vkCmdPushConstants2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushDescriptorSet2KHR(VkCommandBuffer commandBuffer, const VkPushDescriptorSetInfo * pPushDescriptorSetInfo) {
    glad_vkCmdPushDescriptorSet2KHR = (PFN_vkCmdPushDescriptorSet2KHR) glad_vk_on_demand_loader("vkCmdPushDescriptorSet2KHR");
    glad_vkCmdPushDescriptorSet2KHR(commandBuffer, pPushDescriptorSetInfo);
}
PFN_vkCmdPushDescriptorSet2KHR glad_vkCmdPushDescriptorSet2KHR = glad_on_demand_impl_vkCmdPushDescriptorSet2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites) {
    glad_vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR) glad_vk_on_demand_loader("vkCmdPushDescriptorSetKHR");
    glad_vkCmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}
PFN_vkCmdPushDescriptorSetKHR glad_vkCmdPushDescriptorSetKHR = glad_on_demand_impl_vkCmdPushDescriptorSetKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushDescriptorSetWithTemplate2KHR(VkCommandBuffer commandBuffer, const VkPushDescriptorSetWithTemplateInfo * pPushDescriptorSetWithTemplateInfo) {
    glad_vkCmdPushDescriptorSetWithTemplate2KHR = (PFN_vkCmdPushDescriptorSetWithTemplate2KHR) glad_vk_on_demand_loader("vkCmdPushDescriptorSetWithTemplate2KHR");
    glad_vkCmdPushDescriptorSetWithTemplate2KHR(commandBuffer, pPushDescriptorSetWithTemplateInfo);
}
PFN_vkCmdPushDescriptorSetWithTemplate2KHR glad_vkCmdPushDescriptorSetWithTemplate2KHR = glad_on_demand_impl_vkCmdPushDescriptorSetWithTemplate2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void * pData) {
    glad_vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR) glad_vk_on_demand_loader("vkCmdPushDescriptorSetWithTemplateKHR");
    glad_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}
PFN_vkCmdPushDescriptorSetWithTemplateKHR glad_vkCmdPushDescriptorSetWithTemplateKHR = glad_on_demand_impl_vkCmdPushDescriptorSetWithTemplateKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    glad_vkCmdResetEvent = (PFN_vkCmdResetEvent) glad_vk_on_demand_loader("vkCmdResetEvent");
    glad_vkCmdResetEvent(commandBuffer, event, stageMask);
}
PFN_vkCmdResetEvent glad_vkCmdResetEvent = glad_on_demand_impl_vkCmdResetEvent;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResetEvent2(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask) {
    glad_vkCmdResetEvent2 = (PFN_vkCmdResetEvent2) glad_vk_on_demand_loader("vkCmdResetEvent2");
    glad_vkCmdResetEvent2(commandBuffer, event, stageMask);
}
PFN_vkCmdResetEvent2 glad_vkCmdResetEvent2 = glad_on_demand_impl_vkCmdResetEvent2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask) {
    glad_vkCmdResetEvent2KHR = (PFN_vkCmdResetEvent2KHR) glad_vk_on_demand_loader("vkCmdResetEvent2KHR");
    glad_vkCmdResetEvent2KHR(commandBuffer, event, stageMask);
}
PFN_vkCmdResetEvent2KHR glad_vkCmdResetEvent2KHR = glad_on_demand_impl_vkCmdResetEvent2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    glad_vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool) glad_vk_on_demand_loader("vkCmdResetQueryPool");
    glad_vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}
PFN_vkCmdResetQueryPool glad_vkCmdResetQueryPool = glad_on_demand_impl_vkCmdResetQueryPool;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve * pRegions) {
    glad_vkCmdResolveImage = (PFN_vkCmdResolveImage) glad_vk_on_demand_loader("vkCmdResolveImage");
    glad_vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}
PFN_vkCmdResolveImage glad_vkCmdResolveImage = glad_on_demand_impl_vkCmdResolveImage;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 * pResolveImageInfo) {
    glad_vkCmdResolveImage2 = (PFN_vkCmdResolveImage2) glad_vk_on_demand_loader("vkCmdResolveImage2");
    glad_vkCmdResolveImage2(commandBuffer, pResolveImageInfo);
}
PFN_vkCmdResolveImage2 glad_vkCmdResolveImage2 = glad_on_demand_impl_vkCmdResolveImage2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 * pResolveImageInfo) {
    glad_vkCmdResolveImage2KHR = (PFN_vkCmdResolveImage2KHR) glad_vk_on_demand_loader("vkCmdResolveImage2KHR");
    glad_vkCmdResolveImage2KHR(commandBuffer, pResolveImageInfo);
}
PFN_vkCmdResolveImage2KHR glad_vkCmdResolveImage2KHR = glad_on_demand_impl_vkCmdResolveImage2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants [4]) {
    glad_vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants) glad_vk_on_demand_loader("vkCmdSetBlendConstants");
    glad_vkCmdSetBlendConstants(commandBuffer, blendConstants);
}
PFN_vkCmdSetBlendConstants glad_vkCmdSetBlendConstants = glad_on_demand_impl_vkCmdSetBlendConstants;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetCullMode(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode) {
    glad_vkCmdSetCullMode = (PFN_vkCmdSetCullMode) glad_vk_on_demand_loader("vkCmdSetCullMode");
    glad_vkCmdSetCullMode(commandBuffer, cullMode);
}
PFN_vkCmdSetCullMode glad_vkCmdSetCullMode = glad_on_demand_impl_vkCmdSetCullMode;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode) {
    glad_vkCmdSetCullModeEXT = (PFN_vkCmdSetCullModeEXT) glad_vk_on_demand_loader("vkCmdSetCullModeEXT");
    glad_vkCmdSetCullModeEXT(commandBuffer, cullMode);
}
PFN_vkCmdSetCullModeEXT glad_vkCmdSetCullModeEXT = glad_on_demand_impl_vkCmdSetCullModeEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    glad_vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias) glad_vk_on_demand_loader("vkCmdSetDepthBias");
    glad_vkCmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}
PFN_vkCmdSetDepthBias glad_vkCmdSetDepthBias = glad_on_demand_impl_vkCmdSetDepthBias;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBiasEnable(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable) {
    glad_vkCmdSetDepthBiasEnable = (PFN_vkCmdSetDepthBiasEnable) glad_vk_on_demand_loader("vkCmdSetDepthBiasEnable");
    glad_vkCmdSetDepthBiasEnable(commandBuffer, depthBiasEnable);
}
PFN_vkCmdSetDepthBiasEnable glad_vkCmdSetDepthBiasEnable = glad_on_demand_impl_vkCmdSetDepthBiasEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable) {
    glad_vkCmdSetDepthBiasEnableEXT = (PFN_vkCmdSetDepthBiasEnableEXT) glad_vk_on_demand_loader("vkCmdSetDepthBiasEnableEXT");
    glad_vkCmdSetDepthBiasEnableEXT(commandBuffer, depthBiasEnable);
}
PFN_vkCmdSetDepthBiasEnableEXT glad_vkCmdSetDepthBiasEnableEXT = glad_on_demand_impl_vkCmdSetDepthBiasEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    glad_vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds) glad_vk_on_demand_loader("vkCmdSetDepthBounds");
    glad_vkCmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}
PFN_vkCmdSetDepthBounds glad_vkCmdSetDepthBounds = glad_on_demand_impl_vkCmdSetDepthBounds;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBoundsTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable) {
    glad_vkCmdSetDepthBoundsTestEnable = (PFN_vkCmdSetDepthBoundsTestEnable) glad_vk_on_demand_loader("vkCmdSetDepthBoundsTestEnable");
    glad_vkCmdSetDepthBoundsTestEnable(commandBuffer, depthBoundsTestEnable);
}
PFN_vkCmdSetDepthBoundsTestEnable glad_vkCmdSetDepthBoundsTestEnable = glad_on_demand_impl_vkCmdSetDepthBoundsTestEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable) {
    glad_vkCmdSetDepthBoundsTestEnableEXT = (PFN_vkCmdSetDepthBoundsTestEnableEXT) glad_vk_on_demand_loader("vkCmdSetDepthBoundsTestEnableEXT");
    glad_vkCmdSetDepthBoundsTestEnableEXT(commandBuffer, depthBoundsTestEnable);
}
PFN_vkCmdSetDepthBoundsTestEnableEXT glad_vkCmdSetDepthBoundsTestEnableEXT = glad_on_demand_impl_vkCmdSetDepthBoundsTestEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthCompareOp(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp) {
    glad_vkCmdSetDepthCompareOp = (PFN_vkCmdSetDepthCompareOp) glad_vk_on_demand_loader("vkCmdSetDepthCompareOp");
    glad_vkCmdSetDepthCompareOp(commandBuffer, depthCompareOp);
}
PFN_vkCmdSetDepthCompareOp glad_vkCmdSetDepthCompareOp = glad_on_demand_impl_vkCmdSetDepthCompareOp;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp) {
    glad_vkCmdSetDepthCompareOpEXT = (PFN_vkCmdSetDepthCompareOpEXT) glad_vk_on_demand_loader("vkCmdSetDepthCompareOpEXT");
    glad_vkCmdSetDepthCompareOpEXT(commandBuffer, depthCompareOp);
}
PFN_vkCmdSetDepthCompareOpEXT glad_vkCmdSetDepthCompareOpEXT = glad_on_demand_impl_vkCmdSetDepthCompareOpEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable) {
    glad_vkCmdSetDepthTestEnable = (PFN_vkCmdSetDepthTestEnable) glad_vk_on_demand_loader("vkCmdSetDepthTestEnable");
    glad_vkCmdSetDepthTestEnable(commandBuffer, depthTestEnable);
}
PFN_vkCmdSetDepthTestEnable glad_vkCmdSetDepthTestEnable = glad_on_demand_impl_vkCmdSetDepthTestEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable) {
    glad_vkCmdSetDepthTestEnableEXT = (PFN_vkCmdSetDepthTestEnableEXT) glad_vk_on_demand_loader("vkCmdSetDepthTestEnableEXT");
    glad_vkCmdSetDepthTestEnableEXT(commandBuffer, depthTestEnable);
}
PFN_vkCmdSetDepthTestEnableEXT glad_vkCmdSetDepthTestEnableEXT = glad_on_demand_impl_vkCmdSetDepthTestEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthWriteEnable(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable) {
    glad_vkCmdSetDepthWriteEnable = (PFN_vkCmdSetDepthWriteEnable) glad_vk_on_demand_loader("vkCmdSetDepthWriteEnable");
    glad_vkCmdSetDepthWriteEnable(commandBuffer, depthWriteEnable);
}
PFN_vkCmdSetDepthWriteEnable glad_vkCmdSetDepthWriteEnable = glad_on_demand_impl_vkCmdSetDepthWriteEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable) {
    glad_vkCmdSetDepthWriteEnableEXT = (PFN_vkCmdSetDepthWriteEnableEXT) glad_vk_on_demand_loader("vkCmdSetDepthWriteEnableEXT");
    glad_vkCmdSetDepthWriteEnableEXT(commandBuffer, depthWriteEnable);
}
PFN_vkCmdSetDepthWriteEnableEXT glad_vkCmdSetDepthWriteEnableEXT = glad_on_demand_impl_vkCmdSetDepthWriteEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDescriptorBufferOffsets2EXT(VkCommandBuffer commandBuffer, const VkSetDescriptorBufferOffsetsInfoEXT * pSetDescriptorBufferOffsetsInfo) {
    glad_vkCmdSetDescriptorBufferOffsets2EXT = (PFN_vkCmdSetDescriptorBufferOffsets2EXT) glad_vk_on_demand_loader("vkCmdSetDescriptorBufferOffsets2EXT");
    glad_vkCmdSetDescriptorBufferOffsets2EXT(commandBuffer, pSetDescriptorBufferOffsetsInfo);
}
PFN_vkCmdSetDescriptorBufferOffsets2EXT glad_vkCmdSetDescriptorBufferOffsets2EXT = glad_on_demand_impl_vkCmdSetDescriptorBufferOffsets2EXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    glad_vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask) glad_vk_on_demand_loader("vkCmdSetDeviceMask");
    glad_vkCmdSetDeviceMask(commandBuffer, deviceMask);
}
PFN_vkCmdSetDeviceMask glad_vkCmdSetDeviceMask = glad_on_demand_impl_vkCmdSetDeviceMask;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    glad_vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR) glad_vk_on_demand_loader("vkCmdSetDeviceMaskKHR");
    glad_vkCmdSetDeviceMaskKHR(commandBuffer, deviceMask);
}
PFN_vkCmdSetDeviceMaskKHR glad_vkCmdSetDeviceMaskKHR = glad_on_demand_impl_vkCmdSetDeviceMaskKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    glad_vkCmdSetEvent = (PFN_vkCmdSetEvent) glad_vk_on_demand_loader("vkCmdSetEvent");
    glad_vkCmdSetEvent(commandBuffer, event, stageMask);
}
PFN_vkCmdSetEvent glad_vkCmdSetEvent = glad_on_demand_impl_vkCmdSetEvent;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetEvent2(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo * pDependencyInfo) {
    glad_vkCmdSetEvent2 = (PFN_vkCmdSetEvent2) glad_vk_on_demand_loader("vkCmdSetEvent2");
    glad_vkCmdSetEvent2(commandBuffer, event, pDependencyInfo);
}
PFN_vkCmdSetEvent2 glad_vkCmdSetEvent2 = glad_on_demand_impl_vkCmdSetEvent2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo * pDependencyInfo) {
    glad_vkCmdSetEvent2KHR = (PFN_vkCmdSetEvent2KHR) glad_vk_on_demand_loader("vkCmdSetEvent2KHR");
    glad_vkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo);
}
PFN_vkCmdSetEvent2KHR glad_vkCmdSetEvent2KHR = glad_on_demand_impl_vkCmdSetEvent2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetFrontFace(VkCommandBuffer commandBuffer, VkFrontFace frontFace) {
    glad_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFace) glad_vk_on_demand_loader("vkCmdSetFrontFace");
    glad_vkCmdSetFrontFace(commandBuffer, frontFace);
}
PFN_vkCmdSetFrontFace glad_vkCmdSetFrontFace = glad_on_demand_impl_vkCmdSetFrontFace;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace) {
    glad_vkCmdSetFrontFaceEXT = (PFN_vkCmdSetFrontFaceEXT) glad_vk_on_demand_loader("vkCmdSetFrontFaceEXT");
    glad_vkCmdSetFrontFaceEXT(commandBuffer, frontFace);
}
PFN_vkCmdSetFrontFaceEXT glad_vkCmdSetFrontFaceEXT = glad_on_demand_impl_vkCmdSetFrontFaceEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern) {
    glad_vkCmdSetLineStippleEXT = (PFN_vkCmdSetLineStippleEXT) glad_vk_on_demand_loader("vkCmdSetLineStippleEXT");
    glad_vkCmdSetLineStippleEXT(commandBuffer, lineStippleFactor, lineStipplePattern);
}
PFN_vkCmdSetLineStippleEXT glad_vkCmdSetLineStippleEXT = glad_on_demand_impl_vkCmdSetLineStippleEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    glad_vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth) glad_vk_on_demand_loader("vkCmdSetLineWidth");
    glad_vkCmdSetLineWidth(commandBuffer, lineWidth);
}
PFN_vkCmdSetLineWidth glad_vkCmdSetLineWidth = glad_on_demand_impl_vkCmdSetLineWidth;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp) {
    glad_vkCmdSetLogicOpEXT = (PFN_vkCmdSetLogicOpEXT) glad_vk_on_demand_loader("vkCmdSetLogicOpEXT");
    glad_vkCmdSetLogicOpEXT(commandBuffer, logicOp);
}
PFN_vkCmdSetLogicOpEXT glad_vkCmdSetLogicOpEXT = glad_on_demand_impl_vkCmdSetLogicOpEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints) {
    glad_vkCmdSetPatchControlPointsEXT = (PFN_vkCmdSetPatchControlPointsEXT) glad_vk_on_demand_loader("vkCmdSetPatchControlPointsEXT");
    glad_vkCmdSetPatchControlPointsEXT(commandBuffer, patchControlPoints);
}
PFN_vkCmdSetPatchControlPointsEXT glad_vkCmdSetPatchControlPointsEXT = glad_on_demand_impl_vkCmdSetPatchControlPointsEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetPrimitiveRestartEnable(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable) {
    glad_vkCmdSetPrimitiveRestartEnable = (PFN_vkCmdSetPrimitiveRestartEnable) glad_vk_on_demand_loader("vkCmdSetPrimitiveRestartEnable");
    glad_vkCmdSetPrimitiveRestartEnable(commandBuffer, primitiveRestartEnable);
}
PFN_vkCmdSetPrimitiveRestartEnable glad_vkCmdSetPrimitiveRestartEnable = glad_on_demand_impl_vkCmdSetPrimitiveRestartEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable) {
    glad_vkCmdSetPrimitiveRestartEnableEXT = (PFN_vkCmdSetPrimitiveRestartEnableEXT) glad_vk_on_demand_loader("vkCmdSetPrimitiveRestartEnableEXT");
    glad_vkCmdSetPrimitiveRestartEnableEXT(commandBuffer, primitiveRestartEnable);
}
PFN_vkCmdSetPrimitiveRestartEnableEXT glad_vkCmdSetPrimitiveRestartEnableEXT = glad_on_demand_impl_vkCmdSetPrimitiveRestartEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetPrimitiveTopology(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology) {
    glad_vkCmdSetPrimitiveTopology = (PFN_vkCmdSetPrimitiveTopology) glad_vk_on_demand_loader("vkCmdSetPrimitiveTopology");
    glad_vkCmdSetPrimitiveTopology(commandBuffer, primitiveTopology);
}
PFN_vkCmdSetPrimitiveTopology glad_vkCmdSetPrimitiveTopology = glad_on_demand_impl_vkCmdSetPrimitiveTopology;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology) {
    glad_vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT) glad_vk_on_demand_loader("vkCmdSetPrimitiveTopologyEXT");
    glad_vkCmdSetPrimitiveTopologyEXT(commandBuffer, primitiveTopology);
}
PFN_vkCmdSetPrimitiveTopologyEXT glad_vkCmdSetPrimitiveTopologyEXT = glad_on_demand_impl_vkCmdSetPrimitiveTopologyEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetRasterizerDiscardEnable(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable) {
    glad_vkCmdSetRasterizerDiscardEnable = (PFN_vkCmdSetRasterizerDiscardEnable) glad_vk_on_demand_loader("vkCmdSetRasterizerDiscardEnable");
    glad_vkCmdSetRasterizerDiscardEnable(commandBuffer, rasterizerDiscardEnable);
}
PFN_vkCmdSetRasterizerDiscardEnable glad_vkCmdSetRasterizerDiscardEnable = glad_on_demand_impl_vkCmdSetRasterizerDiscardEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable) {
    glad_vkCmdSetRasterizerDiscardEnableEXT = (PFN_vkCmdSetRasterizerDiscardEnableEXT) glad_vk_on_demand_loader("vkCmdSetRasterizerDiscardEnableEXT");
    glad_vkCmdSetRasterizerDiscardEnableEXT(commandBuffer, rasterizerDiscardEnable);
}
PFN_vkCmdSetRasterizerDiscardEnableEXT glad_vkCmdSetRasterizerDiscardEnableEXT = glad_on_demand_impl_vkCmdSetRasterizerDiscardEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetRenderingAttachmentLocationsKHR(VkCommandBuffer commandBuffer, const VkRenderingAttachmentLocationInfo * pLocationInfo) {
    glad_vkCmdSetRenderingAttachmentLocationsKHR = (PFN_vkCmdSetRenderingAttachmentLocationsKHR) glad_vk_on_demand_loader("vkCmdSetRenderingAttachmentLocationsKHR");
    glad_vkCmdSetRenderingAttachmentLocationsKHR(commandBuffer, pLocationInfo);
}
PFN_vkCmdSetRenderingAttachmentLocationsKHR glad_vkCmdSetRenderingAttachmentLocationsKHR = glad_on_demand_impl_vkCmdSetRenderingAttachmentLocationsKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetRenderingInputAttachmentIndicesKHR(VkCommandBuffer commandBuffer, const VkRenderingInputAttachmentIndexInfo * pInputAttachmentIndexInfo) {
    glad_vkCmdSetRenderingInputAttachmentIndicesKHR = (PFN_vkCmdSetRenderingInputAttachmentIndicesKHR) glad_vk_on_demand_loader("vkCmdSetRenderingInputAttachmentIndicesKHR");
    glad_vkCmdSetRenderingInputAttachmentIndicesKHR(commandBuffer, pInputAttachmentIndexInfo);
}
PFN_vkCmdSetRenderingInputAttachmentIndicesKHR glad_vkCmdSetRenderingInputAttachmentIndicesKHR = glad_on_demand_impl_vkCmdSetRenderingInputAttachmentIndicesKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D * pScissors) {
    glad_vkCmdSetScissor = (PFN_vkCmdSetScissor) glad_vk_on_demand_loader("vkCmdSetScissor");
    glad_vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}
PFN_vkCmdSetScissor glad_vkCmdSetScissor = glad_on_demand_impl_vkCmdSetScissor;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetScissorWithCount(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D * pScissors) {
    glad_vkCmdSetScissorWithCount = (PFN_vkCmdSetScissorWithCount) glad_vk_on_demand_loader("vkCmdSetScissorWithCount");
    glad_vkCmdSetScissorWithCount(commandBuffer, scissorCount, pScissors);
}
PFN_vkCmdSetScissorWithCount glad_vkCmdSetScissorWithCount = glad_on_demand_impl_vkCmdSetScissorWithCount;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D * pScissors) {
    glad_vkCmdSetScissorWithCountEXT = (PFN_vkCmdSetScissorWithCountEXT) glad_vk_on_demand_loader("vkCmdSetScissorWithCountEXT");
    glad_vkCmdSetScissorWithCountEXT(commandBuffer, scissorCount, pScissors);
}
PFN_vkCmdSetScissorWithCountEXT glad_vkCmdSetScissorWithCountEXT = glad_on_demand_impl_vkCmdSetScissorWithCountEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
    glad_vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask) glad_vk_on_demand_loader("vkCmdSetStencilCompareMask");
    glad_vkCmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}
PFN_vkCmdSetStencilCompareMask glad_vkCmdSetStencilCompareMask = glad_on_demand_impl_vkCmdSetStencilCompareMask;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilOp(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp) {
    glad_vkCmdSetStencilOp = (PFN_vkCmdSetStencilOp) glad_vk_on_demand_loader("vkCmdSetStencilOp");
    glad_vkCmdSetStencilOp(commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}
PFN_vkCmdSetStencilOp glad_vkCmdSetStencilOp = glad_on_demand_impl_vkCmdSetStencilOp;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp) {
    glad_vkCmdSetStencilOpEXT = (PFN_vkCmdSetStencilOpEXT) glad_vk_on_demand_loader("vkCmdSetStencilOpEXT");
    glad_vkCmdSetStencilOpEXT(commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}
PFN_vkCmdSetStencilOpEXT glad_vkCmdSetStencilOpEXT = glad_on_demand_impl_vkCmdSetStencilOpEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    glad_vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference) glad_vk_on_demand_loader("vkCmdSetStencilReference");
    glad_vkCmdSetStencilReference(commandBuffer, faceMask, reference);
}
PFN_vkCmdSetStencilReference glad_vkCmdSetStencilReference = glad_on_demand_impl_vkCmdSetStencilReference;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilTestEnable(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable) {
    glad_vkCmdSetStencilTestEnable = (PFN_vkCmdSetStencilTestEnable) glad_vk_on_demand_loader("vkCmdSetStencilTestEnable");
    glad_vkCmdSetStencilTestEnable(commandBuffer, stencilTestEnable);
}
PFN_vkCmdSetStencilTestEnable glad_vkCmdSetStencilTestEnable = glad_on_demand_impl_vkCmdSetStencilTestEnable;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable) {
    glad_vkCmdSetStencilTestEnableEXT = (PFN_vkCmdSetStencilTestEnableEXT) glad_vk_on_demand_loader("vkCmdSetStencilTestEnableEXT");
    glad_vkCmdSetStencilTestEnableEXT(commandBuffer, stencilTestEnable);
}
PFN_vkCmdSetStencilTestEnableEXT glad_vkCmdSetStencilTestEnableEXT = glad_on_demand_impl_vkCmdSetStencilTestEnableEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    glad_vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask) glad_vk_on_demand_loader("vkCmdSetStencilWriteMask");
    glad_vkCmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}
PFN_vkCmdSetStencilWriteMask glad_vkCmdSetStencilWriteMask = glad_on_demand_impl_vkCmdSetStencilWriteMask;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT * pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT * pVertexAttributeDescriptions) {
    glad_vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT) glad_vk_on_demand_loader("vkCmdSetVertexInputEXT");
    glad_vkCmdSetVertexInputEXT(commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
}
PFN_vkCmdSetVertexInputEXT glad_vkCmdSetVertexInputEXT = glad_on_demand_impl_vkCmdSetVertexInputEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport * pViewports) {
    glad_vkCmdSetViewport = (PFN_vkCmdSetViewport) glad_vk_on_demand_loader("vkCmdSetViewport");
    glad_vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}
PFN_vkCmdSetViewport glad_vkCmdSetViewport = glad_on_demand_impl_vkCmdSetViewport;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetViewportWithCount(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport * pViewports) {
    glad_vkCmdSetViewportWithCount = (PFN_vkCmdSetViewportWithCount) glad_vk_on_demand_loader("vkCmdSetViewportWithCount");
    glad_vkCmdSetViewportWithCount(commandBuffer, viewportCount, pViewports);
}
PFN_vkCmdSetViewportWithCount glad_vkCmdSetViewportWithCount = glad_on_demand_impl_vkCmdSetViewportWithCount;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport * pViewports) {
    glad_vkCmdSetViewportWithCountEXT = (PFN_vkCmdSetViewportWithCountEXT) glad_vk_on_demand_loader("vkCmdSetViewportWithCountEXT");
    glad_vkCmdSetViewportWithCountEXT(commandBuffer, viewportCount, pViewports);
}
PFN_vkCmdSetViewportWithCountEXT glad_vkCmdSetViewportWithCountEXT = glad_on_demand_impl_vkCmdSetViewportWithCountEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void * pData) {
    glad_vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer) glad_vk_on_demand_loader("vkCmdUpdateBuffer");
    glad_vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}
PFN_vkCmdUpdateBuffer glad_vkCmdUpdateBuffer = glad_on_demand_impl_vkCmdUpdateBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent * pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers) {
    glad_vkCmdWaitEvents = (PFN_vkCmdWaitEvents) glad_vk_on_demand_loader("vkCmdWaitEvents");
    glad_vkCmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}
PFN_vkCmdWaitEvents glad_vkCmdWaitEvents = glad_on_demand_impl_vkCmdWaitEvents;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWaitEvents2(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent * pEvents, const VkDependencyInfo * pDependencyInfos) {
    glad_vkCmdWaitEvents2 = (PFN_vkCmdWaitEvents2) glad_vk_on_demand_loader("vkCmdWaitEvents2");
    glad_vkCmdWaitEvents2(commandBuffer, eventCount, pEvents, pDependencyInfos);
}
PFN_vkCmdWaitEvents2 glad_vkCmdWaitEvents2 = glad_on_demand_impl_vkCmdWaitEvents2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent * pEvents, const VkDependencyInfo * pDependencyInfos) {
    glad_vkCmdWaitEvents2KHR = (PFN_vkCmdWaitEvents2KHR) glad_vk_on_demand_loader("vkCmdWaitEvents2KHR");
    glad_vkCmdWaitEvents2KHR(commandBuffer, eventCount, pEvents, pDependencyInfos);
}
PFN_vkCmdWaitEvents2KHR glad_vkCmdWaitEvents2KHR = glad_on_demand_impl_vkCmdWaitEvents2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) {
    glad_vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp) glad_vk_on_demand_loader("vkCmdWriteTimestamp");
    glad_vkCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}
PFN_vkCmdWriteTimestamp glad_vkCmdWriteTimestamp = glad_on_demand_impl_vkCmdWriteTimestamp;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query) {
    glad_vkCmdWriteTimestamp2 = (PFN_vkCmdWriteTimestamp2) glad_vk_on_demand_loader("vkCmdWriteTimestamp2");
    glad_vkCmdWriteTimestamp2(commandBuffer, stage, queryPool, query);
}
PFN_vkCmdWriteTimestamp2 glad_vkCmdWriteTimestamp2 = glad_on_demand_impl_vkCmdWriteTimestamp2;
static void GLAD_API_PTR glad_on_demand_impl_vkCmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query) {
    glad_vkCmdWriteTimestamp2KHR = (PFN_vkCmdWriteTimestamp2KHR) glad_vk_on_demand_loader("vkCmdWriteTimestamp2KHR");
    glad_vkCmdWriteTimestamp2KHR(commandBuffer, stage, queryPool, query);
}
PFN_vkCmdWriteTimestamp2KHR glad_vkCmdWriteTimestamp2KHR = glad_on_demand_impl_vkCmdWriteTimestamp2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCopyImageToImageEXT(VkDevice device, const VkCopyImageToImageInfo * pCopyImageToImageInfo) {
    glad_vkCopyImageToImageEXT = (PFN_vkCopyImageToImageEXT) glad_vk_on_demand_loader("vkCopyImageToImageEXT");
    return glad_vkCopyImageToImageEXT(device, pCopyImageToImageInfo);
}
PFN_vkCopyImageToImageEXT glad_vkCopyImageToImageEXT = glad_on_demand_impl_vkCopyImageToImageEXT;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCopyImageToMemoryEXT(VkDevice device, const VkCopyImageToMemoryInfo * pCopyImageToMemoryInfo) {
    glad_vkCopyImageToMemoryEXT = (PFN_vkCopyImageToMemoryEXT) glad_vk_on_demand_loader("vkCopyImageToMemoryEXT");
    return glad_vkCopyImageToMemoryEXT(device, pCopyImageToMemoryInfo);
}
PFN_vkCopyImageToMemoryEXT glad_vkCopyImageToMemoryEXT = glad_on_demand_impl_vkCopyImageToMemoryEXT;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCopyMemoryToImageEXT(VkDevice device, const VkCopyMemoryToImageInfo * pCopyMemoryToImageInfo) {
    glad_vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT) glad_vk_on_demand_loader("vkCopyMemoryToImageEXT");
    return glad_vkCopyMemoryToImageEXT(device, pCopyMemoryToImageInfo);
}
PFN_vkCopyMemoryToImageEXT glad_vkCopyMemoryToImageEXT = glad_on_demand_impl_vkCopyMemoryToImageEXT;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSurfaceKHR * pSurface) {
    glad_vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) glad_vk_on_demand_loader("vkCreateAndroidSurfaceKHR");
    return glad_vkCreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}
PFN_vkCreateAndroidSurfaceKHR glad_vkCreateAndroidSurfaceKHR = glad_on_demand_impl_vkCreateAndroidSurfaceKHR;

#endif
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkBuffer * pBuffer) {
    glad_vkCreateBuffer = (PFN_vkCreateBuffer) glad_vk_on_demand_loader("vkCreateBuffer");
    return glad_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}
PFN_vkCreateBuffer glad_vkCreateBuffer = glad_on_demand_impl_vkCreateBuffer;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkBufferView * pView) {
    glad_vkCreateBufferView = (PFN_vkCreateBufferView) glad_vk_on_demand_loader("vkCreateBufferView");
    return glad_vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
}
PFN_vkCreateBufferView glad_vkCreateBufferView = glad_on_demand_impl_vkCreateBufferView;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkCommandPool * pCommandPool) {
    glad_vkCreateCommandPool = (PFN_vkCreateCommandPool) glad_vk_on_demand_loader("vkCreateCommandPool");
    return glad_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}
PFN_vkCreateCommandPool glad_vkCreateCommandPool = glad_on_demand_impl_vkCreateCommandPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator, VkPipeline * pPipelines) {
    glad_vkCreateComputePipelines = (PFN_vkCreateComputePipelines) glad_vk_on_demand_loader("vkCreateComputePipelines");
    return glad_vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}
PFN_vkCreateComputePipelines glad_vkCreateComputePipelines = glad_on_demand_impl_vkCreateComputePipelines;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorPool * pDescriptorPool) {
    glad_vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool) glad_vk_on_demand_loader("vkCreateDescriptorPool");
    return glad_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}
PFN_vkCreateDescriptorPool glad_vkCreateDescriptorPool = glad_on_demand_impl_vkCreateDescriptorPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorSetLayout * pSetLayout) {
    glad_vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) glad_vk_on_demand_loader("vkCreateDescriptorSetLayout");
    return glad_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}
PFN_vkCreateDescriptorSetLayout glad_vkCreateDescriptorSetLayout = glad_on_demand_impl_vkCreateDescriptorSetLayout;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorUpdateTemplate * pDescriptorUpdateTemplate) {
    glad_vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate) glad_vk_on_demand_loader("vkCreateDescriptorUpdateTemplate");
    return glad_vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}
PFN_vkCreateDescriptorUpdateTemplate glad_vkCreateDescriptorUpdateTemplate = glad_on_demand_impl_vkCreateDescriptorUpdateTemplate;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDevice * pDevice) {
    glad_vkCreateDevice = (PFN_vkCreateDevice) glad_vk_on_demand_loader("vkCreateDevice");
    return glad_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}
PFN_vkCreateDevice glad_vkCreateDevice = glad_on_demand_impl_vkCreateDevice;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateEvent(VkDevice device, const VkEventCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkEvent * pEvent) {
    glad_vkCreateEvent = (PFN_vkCreateEvent) glad_vk_on_demand_loader("vkCreateEvent");
    return glad_vkCreateEvent(device, pCreateInfo, pAllocator, pEvent);
}
PFN_vkCreateEvent glad_vkCreateEvent = glad_on_demand_impl_vkCreateEvent;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateFence(VkDevice device, const VkFenceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFence * pFence) {
    glad_vkCreateFence = (PFN_vkCreateFence) glad_vk_on_demand_loader("vkCreateFence");
    return glad_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}
PFN_vkCreateFence glad_vkCreateFence = glad_on_demand_impl_vkCreateFence;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFramebuffer * pFramebuffer) {
    glad_vkCreateFramebuffer = (PFN_vkCreateFramebuffer) glad_vk_on_demand_loader("vkCreateFramebuffer");
    return glad_vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}
PFN_vkCreateFramebuffer glad_vkCreateFramebuffer = glad_on_demand_impl_vkCreateFramebuffer;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator, VkPipeline * pPipelines) {
    glad_vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) glad_vk_on_demand_loader("vkCreateGraphicsPipelines");
    return glad_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}
PFN_vkCreateGraphicsPipelines glad_vkCreateGraphicsPipelines = glad_on_demand_impl_vkCreateGraphicsPipelines;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateImage(VkDevice device, const VkImageCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImage * pImage) {
    glad_vkCreateImage = (PFN_vkCreateImage) glad_vk_on_demand_loader("vkCreateImage");
    return glad_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}
PFN_vkCreateImage glad_vkCreateImage = glad_on_demand_impl_vkCreateImage;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImageView * pView) {
    glad_vkCreateImageView = (PFN_vkCreateImageView) glad_vk_on_demand_loader("vkCreateImageView");
    return glad_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}
PFN_vkCreateImageView glad_vkCreateImageView = glad_on_demand_impl_vkCreateImageView;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateInstance(const VkInstanceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkInstance * pInstance) {
    glad_vkCreateInstance = (PFN_vkCreateInstance) glad_vk_on_demand_loader("vkCreateInstance");
    return glad_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}
PFN_vkCreateInstance glad_vkCreateInstance = glad_on_demand_impl_vkCreateInstance;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkPipelineCache * pPipelineCache) {
    glad_vkCreatePipelineCache = (PFN_vkCreatePipelineCache) glad_vk_on_demand_loader("vkCreatePipelineCache");
    return glad_vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}
PFN_vkCreatePipelineCache glad_vkCreatePipelineCache = glad_on_demand_impl_vkCreatePipelineCache;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkPipelineLayout * pPipelineLayout) {
    glad_vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout) glad_vk_on_demand_loader("vkCreatePipelineLayout");
    return glad_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}
PFN_vkCreatePipelineLayout glad_vkCreatePipelineLayout = glad_on_demand_impl_vkCreatePipelineLayout;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreatePrivateDataSlot(VkDevice device, const VkPrivateDataSlotCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkPrivateDataSlot * pPrivateDataSlot) {
    glad_vkCreatePrivateDataSlot = (PFN_vkCreatePrivateDataSlot) glad_vk_on_demand_loader("vkCreatePrivateDataSlot");
    return glad_vkCreatePrivateDataSlot(device, pCreateInfo, pAllocator, pPrivateDataSlot);
}
PFN_vkCreatePrivateDataSlot glad_vkCreatePrivateDataSlot = glad_on_demand_impl_vkCreatePrivateDataSlot;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkQueryPool * pQueryPool) {
    glad_vkCreateQueryPool = (PFN_vkCreateQueryPool) glad_vk_on_demand_loader("vkCreateQueryPool");
    return glad_vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}
PFN_vkCreateQueryPool glad_vkCreateQueryPool = glad_on_demand_impl_vkCreateQueryPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass) {
    glad_vkCreateRenderPass = (PFN_vkCreateRenderPass) glad_vk_on_demand_loader("vkCreateRenderPass");
    return glad_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}
PFN_vkCreateRenderPass glad_vkCreateRenderPass = glad_on_demand_impl_vkCreateRenderPass;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass) {
    glad_vkCreateRenderPass2 = (PFN_vkCreateRenderPass2) glad_vk_on_demand_loader("vkCreateRenderPass2");
    return glad_vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
}
PFN_vkCreateRenderPass2 glad_vkCreateRenderPass2 = glad_on_demand_impl_vkCreateRenderPass2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass) {
    glad_vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR) glad_vk_on_demand_loader("vkCreateRenderPass2KHR");
    return glad_vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
}
PFN_vkCreateRenderPass2KHR glad_vkCreateRenderPass2KHR = glad_on_demand_impl_vkCreateRenderPass2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSampler * pSampler) {
    glad_vkCreateSampler = (PFN_vkCreateSampler) glad_vk_on_demand_loader("vkCreateSampler");
    return glad_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}
PFN_vkCreateSampler glad_vkCreateSampler = glad_on_demand_impl_vkCreateSampler;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSamplerYcbcrConversion * pYcbcrConversion) {
    glad_vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion) glad_vk_on_demand_loader("vkCreateSamplerYcbcrConversion");
    return glad_vkCreateSamplerYcbcrConversion(device, pCreateInfo, pAllocator, pYcbcrConversion);
}
PFN_vkCreateSamplerYcbcrConversion glad_vkCreateSamplerYcbcrConversion = glad_on_demand_impl_vkCreateSamplerYcbcrConversion;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSamplerYcbcrConversion * pYcbcrConversion) {
    glad_vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR) glad_vk_on_demand_loader("vkCreateSamplerYcbcrConversionKHR");
    return glad_vkCreateSamplerYcbcrConversionKHR(device, pCreateInfo, pAllocator, pYcbcrConversion);
}
PFN_vkCreateSamplerYcbcrConversionKHR glad_vkCreateSamplerYcbcrConversionKHR = glad_on_demand_impl_vkCreateSamplerYcbcrConversionKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSemaphore * pSemaphore) {
    glad_vkCreateSemaphore = (PFN_vkCreateSemaphore) glad_vk_on_demand_loader("vkCreateSemaphore");
    return glad_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}
PFN_vkCreateSemaphore glad_vkCreateSemaphore = glad_on_demand_impl_vkCreateSemaphore;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkShaderModule * pShaderModule) {
    glad_vkCreateShaderModule = (PFN_vkCreateShaderModule) glad_vk_on_demand_loader("vkCreateShaderModule");
    return glad_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}
PFN_vkCreateShaderModule glad_vkCreateShaderModule = glad_on_demand_impl_vkCreateShaderModule;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSwapchainKHR * pSwapchain) {
    glad_vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) glad_vk_on_demand_loader("vkCreateSwapchainKHR");
    return glad_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}
PFN_vkCreateSwapchainKHR glad_vkCreateSwapchainKHR = glad_on_demand_impl_vkCreateSwapchainKHR;
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSurfaceKHR * pSurface) {
    glad_vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) glad_vk_on_demand_loader("vkCreateWaylandSurfaceKHR");
    return glad_vkCreateWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}
PFN_vkCreateWaylandSurfaceKHR glad_vkCreateWaylandSurfaceKHR = glad_on_demand_impl_vkCreateWaylandSurfaceKHR;

#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSurfaceKHR * pSurface) {
    glad_vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) glad_vk_on_demand_loader("vkCreateWin32SurfaceKHR");
    return glad_vkCreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}
PFN_vkCreateWin32SurfaceKHR glad_vkCreateWin32SurfaceKHR = glad_on_demand_impl_vkCreateWin32SurfaceKHR;

#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSurfaceKHR * pSurface) {
    glad_vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) glad_vk_on_demand_loader("vkCreateXcbSurfaceKHR");
    return glad_vkCreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}
PFN_vkCreateXcbSurfaceKHR glad_vkCreateXcbSurfaceKHR = glad_on_demand_impl_vkCreateXcbSurfaceKHR;

#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSurfaceKHR * pSurface) {
    glad_vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) glad_vk_on_demand_loader("vkCreateXlibSurfaceKHR");
    return glad_vkCreateXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}
PFN_vkCreateXlibSurfaceKHR glad_vkCreateXlibSurfaceKHR = glad_on_demand_impl_vkCreateXlibSurfaceKHR;

#endif
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyBuffer = (PFN_vkDestroyBuffer) glad_vk_on_demand_loader("vkDestroyBuffer");
    glad_vkDestroyBuffer(device, buffer, pAllocator);
}
PFN_vkDestroyBuffer glad_vkDestroyBuffer = glad_on_demand_impl_vkDestroyBuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyBufferView = (PFN_vkDestroyBufferView) glad_vk_on_demand_loader("vkDestroyBufferView");
    glad_vkDestroyBufferView(device, bufferView, pAllocator);
}
PFN_vkDestroyBufferView glad_vkDestroyBufferView = glad_on_demand_impl_vkDestroyBufferView;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyCommandPool = (PFN_vkDestroyCommandPool) glad_vk_on_demand_loader("vkDestroyCommandPool");
    glad_vkDestroyCommandPool(device, commandPool, pAllocator);
}
PFN_vkDestroyCommandPool glad_vkDestroyCommandPool = glad_on_demand_impl_vkDestroyCommandPool;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool) glad_vk_on_demand_loader("vkDestroyDescriptorPool");
    glad_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}
PFN_vkDestroyDescriptorPool glad_vkDestroyDescriptorPool = glad_on_demand_impl_vkDestroyDescriptorPool;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout) glad_vk_on_demand_loader("vkDestroyDescriptorSetLayout");
    glad_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}
PFN_vkDestroyDescriptorSetLayout glad_vkDestroyDescriptorSetLayout = glad_on_demand_impl_vkDestroyDescriptorSetLayout;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate) glad_vk_on_demand_loader("vkDestroyDescriptorUpdateTemplate");
    glad_vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
}
PFN_vkDestroyDescriptorUpdateTemplate glad_vkDestroyDescriptorUpdateTemplate = glad_on_demand_impl_vkDestroyDescriptorUpdateTemplate;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyDevice = (PFN_vkDestroyDevice) glad_vk_on_demand_loader("vkDestroyDevice");
    glad_vkDestroyDevice(device, pAllocator);
}
PFN_vkDestroyDevice glad_vkDestroyDevice = glad_on_demand_impl_vkDestroyDevice;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyEvent = (PFN_vkDestroyEvent) glad_vk_on_demand_loader("vkDestroyEvent");
    glad_vkDestroyEvent(device, event, pAllocator);
}
PFN_vkDestroyEvent glad_vkDestroyEvent = glad_on_demand_impl_vkDestroyEvent;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyFence = (PFN_vkDestroyFence) glad_vk_on_demand_loader("vkDestroyFence");
    glad_vkDestroyFence(device, fence, pAllocator);
}
PFN_vkDestroyFence glad_vkDestroyFence = glad_on_demand_impl_vkDestroyFence;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer) glad_vk_on_demand_loader("vkDestroyFramebuffer");
    glad_vkDestroyFramebuffer(device, framebuffer, pAllocator);
}
PFN_vkDestroyFramebuffer glad_vkDestroyFramebuffer = glad_on_demand_impl_vkDestroyFramebuffer;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyImage = (PFN_vkDestroyImage) glad_vk_on_demand_loader("vkDestroyImage");
    glad_vkDestroyImage(device, image, pAllocator);
}
PFN_vkDestroyImage glad_vkDestroyImage = glad_on_demand_impl_vkDestroyImage;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyImageView = (PFN_vkDestroyImageView) glad_vk_on_demand_loader("vkDestroyImageView");
    glad_vkDestroyImageView(device, imageView, pAllocator);
}
PFN_vkDestroyImageView glad_vkDestroyImageView = glad_on_demand_impl_vkDestroyImageView;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyInstance = (PFN_vkDestroyInstance) glad_vk_on_demand_loader("vkDestroyInstance");
    glad_vkDestroyInstance(instance, pAllocator);
}
PFN_vkDestroyInstance glad_vkDestroyInstance = glad_on_demand_impl_vkDestroyInstance;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyPipeline = (PFN_vkDestroyPipeline) glad_vk_on_demand_loader("vkDestroyPipeline");
    glad_vkDestroyPipeline(device, pipeline, pAllocator);
}
PFN_vkDestroyPipeline glad_vkDestroyPipeline = glad_on_demand_impl_vkDestroyPipeline;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache) glad_vk_on_demand_loader("vkDestroyPipelineCache");
    glad_vkDestroyPipelineCache(device, pipelineCache, pAllocator);
}
PFN_vkDestroyPipelineCache glad_vkDestroyPipelineCache = glad_on_demand_impl_vkDestroyPipelineCache;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) glad_vk_on_demand_loader("vkDestroyPipelineLayout");
    glad_vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}
PFN_vkDestroyPipelineLayout glad_vkDestroyPipelineLayout = glad_on_demand_impl_vkDestroyPipelineLayout;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyPrivateDataSlot(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyPrivateDataSlot = (PFN_vkDestroyPrivateDataSlot) glad_vk_on_demand_loader("vkDestroyPrivateDataSlot");
    glad_vkDestroyPrivateDataSlot(device, privateDataSlot, pAllocator);
}
PFN_vkDestroyPrivateDataSlot glad_vkDestroyPrivateDataSlot = glad_on_demand_impl_vkDestroyPrivateDataSlot;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyQueryPool = (PFN_vkDestroyQueryPool) glad_vk_on_demand_loader("vkDestroyQueryPool");
    glad_vkDestroyQueryPool(device, queryPool, pAllocator);
}
PFN_vkDestroyQueryPool glad_vkDestroyQueryPool = glad_on_demand_impl_vkDestroyQueryPool;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyRenderPass = (PFN_vkDestroyRenderPass) glad_vk_on_demand_loader("vkDestroyRenderPass");
    glad_vkDestroyRenderPass(device, renderPass, pAllocator);
}
PFN_vkDestroyRenderPass glad_vkDestroyRenderPass = glad_on_demand_impl_vkDestroyRenderPass;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySampler = (PFN_vkDestroySampler) glad_vk_on_demand_loader("vkDestroySampler");
    glad_vkDestroySampler(device, sampler, pAllocator);
}
PFN_vkDestroySampler glad_vkDestroySampler = glad_on_demand_impl_vkDestroySampler;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion) glad_vk_on_demand_loader("vkDestroySamplerYcbcrConversion");
    glad_vkDestroySamplerYcbcrConversion(device, ycbcrConversion, pAllocator);
}
PFN_vkDestroySamplerYcbcrConversion glad_vkDestroySamplerYcbcrConversion = glad_on_demand_impl_vkDestroySamplerYcbcrConversion;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR) glad_vk_on_demand_loader("vkDestroySamplerYcbcrConversionKHR");
    glad_vkDestroySamplerYcbcrConversionKHR(device, ycbcrConversion, pAllocator);
}
PFN_vkDestroySamplerYcbcrConversionKHR glad_vkDestroySamplerYcbcrConversionKHR = glad_on_demand_impl_vkDestroySamplerYcbcrConversionKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySemaphore = (PFN_vkDestroySemaphore) glad_vk_on_demand_loader("vkDestroySemaphore");
    glad_vkDestroySemaphore(device, semaphore, pAllocator);
}
PFN_vkDestroySemaphore glad_vkDestroySemaphore = glad_on_demand_impl_vkDestroySemaphore;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroyShaderModule = (PFN_vkDestroyShaderModule) glad_vk_on_demand_loader("vkDestroyShaderModule");
    glad_vkDestroyShaderModule(device, shaderModule, pAllocator);
}
PFN_vkDestroyShaderModule glad_vkDestroyShaderModule = glad_on_demand_impl_vkDestroyShaderModule;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) glad_vk_on_demand_loader("vkDestroySurfaceKHR");
    glad_vkDestroySurfaceKHR(instance, surface, pAllocator);
}
PFN_vkDestroySurfaceKHR glad_vkDestroySurfaceKHR = glad_on_demand_impl_vkDestroySurfaceKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks * pAllocator) {
    glad_vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) glad_vk_on_demand_loader("vkDestroySwapchainKHR");
    glad_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}
PFN_vkDestroySwapchainKHR glad_vkDestroySwapchainKHR = glad_on_demand_impl_vkDestroySwapchainKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkDeviceWaitIdle(VkDevice device) {
    glad_vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle) glad_vk_on_demand_loader("vkDeviceWaitIdle");
    return glad_vkDeviceWaitIdle(device);
}
PFN_vkDeviceWaitIdle glad_vkDeviceWaitIdle = glad_on_demand_impl_vkDeviceWaitIdle;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
    glad_vkEndCommandBuffer = (PFN_vkEndCommandBuffer) glad_vk_on_demand_loader("vkEndCommandBuffer");
    return glad_vkEndCommandBuffer(commandBuffer);
}
PFN_vkEndCommandBuffer glad_vkEndCommandBuffer = glad_on_demand_impl_vkEndCommandBuffer;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties) {
    glad_vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) glad_vk_on_demand_loader("vkEnumerateDeviceExtensionProperties");
    return glad_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}
PFN_vkEnumerateDeviceExtensionProperties glad_vkEnumerateDeviceExtensionProperties = glad_on_demand_impl_vkEnumerateDeviceExtensionProperties;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t * pPropertyCount, VkLayerProperties * pProperties) {
    glad_vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) glad_vk_on_demand_loader("vkEnumerateDeviceLayerProperties");
    return glad_vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}
PFN_vkEnumerateDeviceLayerProperties glad_vkEnumerateDeviceLayerProperties = glad_on_demand_impl_vkEnumerateDeviceLayerProperties;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumerateInstanceExtensionProperties(const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties) {
    glad_vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) glad_vk_on_demand_loader("vkEnumerateInstanceExtensionProperties");
    return glad_vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}
PFN_vkEnumerateInstanceExtensionProperties glad_vkEnumerateInstanceExtensionProperties = glad_on_demand_impl_vkEnumerateInstanceExtensionProperties;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumerateInstanceLayerProperties(uint32_t * pPropertyCount, VkLayerProperties * pProperties) {
    glad_vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) glad_vk_on_demand_loader("vkEnumerateInstanceLayerProperties");
    return glad_vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}
PFN_vkEnumerateInstanceLayerProperties glad_vkEnumerateInstanceLayerProperties = glad_on_demand_impl_vkEnumerateInstanceLayerProperties;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumerateInstanceVersion(uint32_t * pApiVersion) {
    glad_vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion) glad_vk_on_demand_loader("vkEnumerateInstanceVersion");
    return glad_vkEnumerateInstanceVersion(pApiVersion);
}
PFN_vkEnumerateInstanceVersion glad_vkEnumerateInstanceVersion = glad_on_demand_impl_vkEnumerateInstanceVersion;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t * pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties * pPhysicalDeviceGroupProperties) {
    glad_vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups) glad_vk_on_demand_loader("vkEnumeratePhysicalDeviceGroups");
    return glad_vkEnumeratePhysicalDeviceGroups(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}
PFN_vkEnumeratePhysicalDeviceGroups glad_vkEnumeratePhysicalDeviceGroups = glad_on_demand_impl_vkEnumeratePhysicalDeviceGroups;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevices) {
    glad_vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) glad_vk_on_demand_loader("vkEnumeratePhysicalDevices");
    return glad_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}
PFN_vkEnumeratePhysicalDevices glad_vkEnumeratePhysicalDevices = glad_on_demand_impl_vkEnumeratePhysicalDevices;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges) {
    glad_vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges) glad_vk_on_demand_loader("vkFlushMappedMemoryRanges");
    return glad_vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}
PFN_vkFlushMappedMemoryRanges glad_vkFlushMappedMemoryRanges = glad_on_demand_impl_vkFlushMappedMemoryRanges;
static void GLAD_API_PTR glad_on_demand_impl_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers) {
    glad_vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers) glad_vk_on_demand_loader("vkFreeCommandBuffers");
    glad_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}
PFN_vkFreeCommandBuffers glad_vkFreeCommandBuffers = glad_on_demand_impl_vkFreeCommandBuffers;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet * pDescriptorSets) {
    glad_vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets) glad_vk_on_demand_loader("vkFreeDescriptorSets");
    return glad_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}
PFN_vkFreeDescriptorSets glad_vkFreeDescriptorSets = glad_on_demand_impl_vkFreeDescriptorSets;
static void GLAD_API_PTR glad_on_demand_impl_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks * pAllocator) {
    glad_vkFreeMemory = (PFN_vkFreeMemory) glad_vk_on_demand_loader("vkFreeMemory");
    glad_vkFreeMemory(device, memory, pAllocator);
}
PFN_vkFreeMemory glad_vkFreeMemory = glad_on_demand_impl_vkFreeMemory;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetAndroidHardwareBufferPropertiesANDROID(VkDevice device, const struct AHardwareBuffer * buffer, VkAndroidHardwareBufferPropertiesANDROID * pProperties) {
    glad_vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID) glad_vk_on_demand_loader("vkGetAndroidHardwareBufferPropertiesANDROID");
    return glad_vkGetAndroidHardwareBufferPropertiesANDROID(device, buffer, pProperties);
}
PFN_vkGetAndroidHardwareBufferPropertiesANDROID glad_vkGetAndroidHardwareBufferPropertiesANDROID = glad_on_demand_impl_vkGetAndroidHardwareBufferPropertiesANDROID;

#endif
static VkDeviceAddress GLAD_API_PTR glad_on_demand_impl_vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo * pInfo) {
    glad_vkGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress) glad_vk_on_demand_loader("vkGetBufferDeviceAddress");
    return glad_vkGetBufferDeviceAddress(device, pInfo);
}
PFN_vkGetBufferDeviceAddress glad_vkGetBufferDeviceAddress = glad_on_demand_impl_vkGetBufferDeviceAddress;
static VkDeviceAddress GLAD_API_PTR glad_on_demand_impl_vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo * pInfo) {
    glad_vkGetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT) glad_vk_on_demand_loader("vkGetBufferDeviceAddressEXT");
    return glad_vkGetBufferDeviceAddressEXT(device, pInfo);
}
PFN_vkGetBufferDeviceAddressEXT glad_vkGetBufferDeviceAddressEXT = glad_on_demand_impl_vkGetBufferDeviceAddressEXT;
static VkDeviceAddress GLAD_API_PTR glad_on_demand_impl_vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo * pInfo) {
    glad_vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR) glad_vk_on_demand_loader("vkGetBufferDeviceAddressKHR");
    return glad_vkGetBufferDeviceAddressKHR(device, pInfo);
}
PFN_vkGetBufferDeviceAddressKHR glad_vkGetBufferDeviceAddressKHR = glad_on_demand_impl_vkGetBufferDeviceAddressKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements) {
    glad_vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements) glad_vk_on_demand_loader("vkGetBufferMemoryRequirements");
    glad_vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}
PFN_vkGetBufferMemoryRequirements glad_vkGetBufferMemoryRequirements = glad_on_demand_impl_vkGetBufferMemoryRequirements;
static void GLAD_API_PTR glad_on_demand_impl_vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2) glad_vk_on_demand_loader("vkGetBufferMemoryRequirements2");
    glad_vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}
PFN_vkGetBufferMemoryRequirements2 glad_vkGetBufferMemoryRequirements2 = glad_on_demand_impl_vkGetBufferMemoryRequirements2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR) glad_vk_on_demand_loader("vkGetBufferMemoryRequirements2KHR");
    glad_vkGetBufferMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}
PFN_vkGetBufferMemoryRequirements2KHR glad_vkGetBufferMemoryRequirements2KHR = glad_on_demand_impl_vkGetBufferMemoryRequirements2KHR;
static uint64_t GLAD_API_PTR glad_on_demand_impl_vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo * pInfo) {
    glad_vkGetBufferOpaqueCaptureAddress = (PFN_vkGetBufferOpaqueCaptureAddress) glad_vk_on_demand_loader("vkGetBufferOpaqueCaptureAddress");
    return glad_vkGetBufferOpaqueCaptureAddress(device, pInfo);
}
PFN_vkGetBufferOpaqueCaptureAddress glad_vkGetBufferOpaqueCaptureAddress = glad_on_demand_impl_vkGetBufferOpaqueCaptureAddress;
static uint64_t GLAD_API_PTR glad_on_demand_impl_vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo * pInfo) {
    glad_vkGetBufferOpaqueCaptureAddressKHR = (PFN_vkGetBufferOpaqueCaptureAddressKHR) glad_vk_on_demand_loader("vkGetBufferOpaqueCaptureAddressKHR");
    return glad_vkGetBufferOpaqueCaptureAddressKHR(device, pInfo);
}
PFN_vkGetBufferOpaqueCaptureAddressKHR glad_vkGetBufferOpaqueCaptureAddressKHR = glad_on_demand_impl_vkGetBufferOpaqueCaptureAddressKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo, VkDescriptorSetLayoutSupport * pSupport) {
    glad_vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport) glad_vk_on_demand_loader("vkGetDescriptorSetLayoutSupport");
    glad_vkGetDescriptorSetLayoutSupport(device, pCreateInfo, pSupport);
}
PFN_vkGetDescriptorSetLayoutSupport glad_vkGetDescriptorSetLayoutSupport = glad_on_demand_impl_vkGetDescriptorSetLayoutSupport;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetDeviceBufferMemoryRequirements = (PFN_vkGetDeviceBufferMemoryRequirements) glad_vk_on_demand_loader("vkGetDeviceBufferMemoryRequirements");
    glad_vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}
PFN_vkGetDeviceBufferMemoryRequirements glad_vkGetDeviceBufferMemoryRequirements = glad_on_demand_impl_vkGetDeviceBufferMemoryRequirements;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetDeviceFaultInfoEXT(VkDevice device, VkDeviceFaultCountsEXT * pFaultCounts, VkDeviceFaultInfoEXT * pFaultInfo) {
    glad_vkGetDeviceFaultInfoEXT = (PFN_vkGetDeviceFaultInfoEXT) glad_vk_on_demand_loader("vkGetDeviceFaultInfoEXT");
    return glad_vkGetDeviceFaultInfoEXT(device, pFaultCounts, pFaultInfo);
}
PFN_vkGetDeviceFaultInfoEXT glad_vkGetDeviceFaultInfoEXT = glad_on_demand_impl_vkGetDeviceFaultInfoEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags * pPeerMemoryFeatures) {
    glad_vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures) glad_vk_on_demand_loader("vkGetDeviceGroupPeerMemoryFeatures");
    glad_vkGetDeviceGroupPeerMemoryFeatures(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}
PFN_vkGetDeviceGroupPeerMemoryFeatures glad_vkGetDeviceGroupPeerMemoryFeatures = glad_on_demand_impl_vkGetDeviceGroupPeerMemoryFeatures;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags * pPeerMemoryFeatures) {
    glad_vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR) glad_vk_on_demand_loader("vkGetDeviceGroupPeerMemoryFeaturesKHR");
    glad_vkGetDeviceGroupPeerMemoryFeaturesKHR(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}
PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR glad_vkGetDeviceGroupPeerMemoryFeaturesKHR = glad_on_demand_impl_vkGetDeviceGroupPeerMemoryFeaturesKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR * pDeviceGroupPresentCapabilities) {
    glad_vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR) glad_vk_on_demand_loader("vkGetDeviceGroupPresentCapabilitiesKHR");
    return glad_vkGetDeviceGroupPresentCapabilitiesKHR(device, pDeviceGroupPresentCapabilities);
}
PFN_vkGetDeviceGroupPresentCapabilitiesKHR glad_vkGetDeviceGroupPresentCapabilitiesKHR = glad_on_demand_impl_vkGetDeviceGroupPresentCapabilitiesKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR * pModes) {
    glad_vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR) glad_vk_on_demand_loader("vkGetDeviceGroupSurfacePresentModesKHR");
    return glad_vkGetDeviceGroupSurfacePresentModesKHR(device, surface, pModes);
}
PFN_vkGetDeviceGroupSurfacePresentModesKHR glad_vkGetDeviceGroupSurfacePresentModesKHR = glad_on_demand_impl_vkGetDeviceGroupSurfacePresentModesKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetDeviceImageMemoryRequirements = (PFN_vkGetDeviceImageMemoryRequirements) glad_vk_on_demand_loader("vkGetDeviceImageMemoryRequirements");
    glad_vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}
PFN_vkGetDeviceImageMemoryRequirements glad_vkGetDeviceImageMemoryRequirements = glad_on_demand_impl_vkGetDeviceImageMemoryRequirements;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceImageSparseMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements * pInfo, uint32_t * pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements) {
    glad_vkGetDeviceImageSparseMemoryRequirements = (PFN_vkGetDeviceImageSparseMemoryRequirements) glad_vk_on_demand_loader("vkGetDeviceImageSparseMemoryRequirements");
    glad_vkGetDeviceImageSparseMemoryRequirements(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}
PFN_vkGetDeviceImageSparseMemoryRequirements glad_vkGetDeviceImageSparseMemoryRequirements = glad_on_demand_impl_vkGetDeviceImageSparseMemoryRequirements;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceImageSubresourceLayoutKHR(VkDevice device, const VkDeviceImageSubresourceInfo * pInfo, VkSubresourceLayout2 * pLayout) {
    glad_vkGetDeviceImageSubresourceLayoutKHR = (PFN_vkGetDeviceImageSubresourceLayoutKHR) glad_vk_on_demand_loader("vkGetDeviceImageSubresourceLayoutKHR");
    glad_vkGetDeviceImageSubresourceLayoutKHR(device, pInfo, pLayout);
}
PFN_vkGetDeviceImageSubresourceLayoutKHR glad_vkGetDeviceImageSubresourceLayoutKHR = glad_on_demand_impl_vkGetDeviceImageSubresourceLayoutKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize * pCommittedMemoryInBytes) {
    glad_vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment) glad_vk_on_demand_loader("vkGetDeviceMemoryCommitment");
    glad_vkGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}
PFN_vkGetDeviceMemoryCommitment glad_vkGetDeviceMemoryCommitment = glad_on_demand_impl_vkGetDeviceMemoryCommitment;
static uint64_t GLAD_API_PTR glad_on_demand_impl_vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo * pInfo) {
    glad_vkGetDeviceMemoryOpaqueCaptureAddress = (PFN_vkGetDeviceMemoryOpaqueCaptureAddress) glad_vk_on_demand_loader("vkGetDeviceMemoryOpaqueCaptureAddress");
    return glad_vkGetDeviceMemoryOpaqueCaptureAddress(device, pInfo);
}
PFN_vkGetDeviceMemoryOpaqueCaptureAddress glad_vkGetDeviceMemoryOpaqueCaptureAddress = glad_on_demand_impl_vkGetDeviceMemoryOpaqueCaptureAddress;
static uint64_t GLAD_API_PTR glad_on_demand_impl_vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo * pInfo) {
    glad_vkGetDeviceMemoryOpaqueCaptureAddressKHR = (PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR) glad_vk_on_demand_loader("vkGetDeviceMemoryOpaqueCaptureAddressKHR");
    return glad_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device, pInfo);
}
PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR glad_vkGetDeviceMemoryOpaqueCaptureAddressKHR = glad_on_demand_impl_vkGetDeviceMemoryOpaqueCaptureAddressKHR;
static PFN_vkVoidFunction GLAD_API_PTR glad_on_demand_impl_vkGetDeviceProcAddr(VkDevice device, const char * pName) {
    glad_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) glad_vk_on_demand_loader("vkGetDeviceProcAddr");
    return glad_vkGetDeviceProcAddr(device, pName);
}
PFN_vkGetDeviceProcAddr glad_vkGetDeviceProcAddr = glad_on_demand_impl_vkGetDeviceProcAddr;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue * pQueue) {
    glad_vkGetDeviceQueue = (PFN_vkGetDeviceQueue) glad_vk_on_demand_loader("vkGetDeviceQueue");
    glad_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
PFN_vkGetDeviceQueue glad_vkGetDeviceQueue = glad_on_demand_impl_vkGetDeviceQueue;
static void GLAD_API_PTR glad_on_demand_impl_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 * pQueueInfo, VkQueue * pQueue) {
    glad_vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2) glad_vk_on_demand_loader("vkGetDeviceQueue2");
    glad_vkGetDeviceQueue2(device, pQueueInfo, pQueue);
}
PFN_vkGetDeviceQueue2 glad_vkGetDeviceQueue2 = glad_on_demand_impl_vkGetDeviceQueue2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetEventStatus(VkDevice device, VkEvent event) {
    glad_vkGetEventStatus = (PFN_vkGetEventStatus) glad_vk_on_demand_loader("vkGetEventStatus");
    return glad_vkGetEventStatus(device, event);
}
PFN_vkGetEventStatus glad_vkGetEventStatus = glad_on_demand_impl_vkGetEventStatus;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetFenceStatus(VkDevice device, VkFence fence) {
    glad_vkGetFenceStatus = (PFN_vkGetFenceStatus) glad_vk_on_demand_loader("vkGetFenceStatus");
    return glad_vkGetFenceStatus(device, fence);
}
PFN_vkGetFenceStatus glad_vkGetFenceStatus = glad_on_demand_impl_vkGetFenceStatus;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements * pMemoryRequirements) {
    glad_vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements) glad_vk_on_demand_loader("vkGetImageMemoryRequirements");
    glad_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}
PFN_vkGetImageMemoryRequirements glad_vkGetImageMemoryRequirements = glad_on_demand_impl_vkGetImageMemoryRequirements;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2) glad_vk_on_demand_loader("vkGetImageMemoryRequirements2");
    glad_vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}
PFN_vkGetImageMemoryRequirements2 glad_vkGetImageMemoryRequirements2 = glad_on_demand_impl_vkGetImageMemoryRequirements2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 * pInfo, VkMemoryRequirements2 * pMemoryRequirements) {
    glad_vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR) glad_vk_on_demand_loader("vkGetImageMemoryRequirements2KHR");
    glad_vkGetImageMemoryRequirements2KHR(device, pInfo, pMemoryRequirements);
}
PFN_vkGetImageMemoryRequirements2KHR glad_vkGetImageMemoryRequirements2KHR = glad_on_demand_impl_vkGetImageMemoryRequirements2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t * pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements * pSparseMemoryRequirements) {
    glad_vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements) glad_vk_on_demand_loader("vkGetImageSparseMemoryRequirements");
    glad_vkGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}
PFN_vkGetImageSparseMemoryRequirements glad_vkGetImageSparseMemoryRequirements = glad_on_demand_impl_vkGetImageSparseMemoryRequirements;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 * pInfo, uint32_t * pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements) {
    glad_vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2) glad_vk_on_demand_loader("vkGetImageSparseMemoryRequirements2");
    glad_vkGetImageSparseMemoryRequirements2(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}
PFN_vkGetImageSparseMemoryRequirements2 glad_vkGetImageSparseMemoryRequirements2 = glad_on_demand_impl_vkGetImageSparseMemoryRequirements2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 * pInfo, uint32_t * pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements) {
    glad_vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR) glad_vk_on_demand_loader("vkGetImageSparseMemoryRequirements2KHR");
    glad_vkGetImageSparseMemoryRequirements2KHR(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}
PFN_vkGetImageSparseMemoryRequirements2KHR glad_vkGetImageSparseMemoryRequirements2KHR = glad_on_demand_impl_vkGetImageSparseMemoryRequirements2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource * pSubresource, VkSubresourceLayout * pLayout) {
    glad_vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) glad_vk_on_demand_loader("vkGetImageSubresourceLayout");
    glad_vkGetImageSubresourceLayout(device, image, pSubresource, pLayout);
}
PFN_vkGetImageSubresourceLayout glad_vkGetImageSubresourceLayout = glad_on_demand_impl_vkGetImageSubresourceLayout;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSubresourceLayout2EXT(VkDevice device, VkImage image, const VkImageSubresource2 * pSubresource, VkSubresourceLayout2 * pLayout) {
    glad_vkGetImageSubresourceLayout2EXT = (PFN_vkGetImageSubresourceLayout2EXT) glad_vk_on_demand_loader("vkGetImageSubresourceLayout2EXT");
    glad_vkGetImageSubresourceLayout2EXT(device, image, pSubresource, pLayout);
}
PFN_vkGetImageSubresourceLayout2EXT glad_vkGetImageSubresourceLayout2EXT = glad_on_demand_impl_vkGetImageSubresourceLayout2EXT;
static void GLAD_API_PTR glad_on_demand_impl_vkGetImageSubresourceLayout2KHR(VkDevice device, VkImage image, const VkImageSubresource2 * pSubresource, VkSubresourceLayout2 * pLayout) {
    glad_vkGetImageSubresourceLayout2KHR = (PFN_vkGetImageSubresourceLayout2KHR) glad_vk_on_demand_loader("vkGetImageSubresourceLayout2KHR");
    glad_vkGetImageSubresourceLayout2KHR(device, image, pSubresource, pLayout);
}
PFN_vkGetImageSubresourceLayout2KHR glad_vkGetImageSubresourceLayout2KHR = glad_on_demand_impl_vkGetImageSubresourceLayout2KHR;
static PFN_vkVoidFunction GLAD_API_PTR glad_on_demand_impl_vkGetInstanceProcAddr(VkInstance instance, const char * pName) {
    glad_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) glad_vk_on_demand_loader("vkGetInstanceProcAddr");
    return glad_vkGetInstanceProcAddr(instance, pName);
}
PFN_vkGetInstanceProcAddr glad_vkGetInstanceProcAddr = glad_on_demand_impl_vkGetInstanceProcAddr;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetMemoryAndroidHardwareBufferANDROID(VkDevice device, const VkMemoryGetAndroidHardwareBufferInfoANDROID * pInfo, struct AHardwareBuffer ** pBuffer) {
    glad_vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID) glad_vk_on_demand_loader("vkGetMemoryAndroidHardwareBufferANDROID");
    return glad_vkGetMemoryAndroidHardwareBufferANDROID(device, pInfo, pBuffer);
}
PFN_vkGetMemoryAndroidHardwareBufferANDROID glad_vkGetMemoryAndroidHardwareBufferANDROID = glad_on_demand_impl_vkGetMemoryAndroidHardwareBufferANDROID;

#endif
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo, VkExternalBufferProperties * pExternalBufferProperties) {
    glad_vkGetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceExternalBufferProperties");
    glad_vkGetPhysicalDeviceExternalBufferProperties(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}
PFN_vkGetPhysicalDeviceExternalBufferProperties glad_vkGetPhysicalDeviceExternalBufferProperties = glad_on_demand_impl_vkGetPhysicalDeviceExternalBufferProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo, VkExternalBufferProperties * pExternalBufferProperties) {
    glad_vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceExternalBufferPropertiesKHR");
    glad_vkGetPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}
PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR glad_vkGetPhysicalDeviceExternalBufferPropertiesKHR = glad_on_demand_impl_vkGetPhysicalDeviceExternalBufferPropertiesKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo * pExternalFenceInfo, VkExternalFenceProperties * pExternalFenceProperties) {
    glad_vkGetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceExternalFenceProperties");
    glad_vkGetPhysicalDeviceExternalFenceProperties(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}
PFN_vkGetPhysicalDeviceExternalFenceProperties glad_vkGetPhysicalDeviceExternalFenceProperties = glad_on_demand_impl_vkGetPhysicalDeviceExternalFenceProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo * pExternalSemaphoreInfo, VkExternalSemaphoreProperties * pExternalSemaphoreProperties) {
    glad_vkGetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceExternalSemaphoreProperties");
    glad_vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}
PFN_vkGetPhysicalDeviceExternalSemaphoreProperties glad_vkGetPhysicalDeviceExternalSemaphoreProperties = glad_on_demand_impl_vkGetPhysicalDeviceExternalSemaphoreProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures * pFeatures) {
    glad_vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) glad_vk_on_demand_loader("vkGetPhysicalDeviceFeatures");
    glad_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}
PFN_vkGetPhysicalDeviceFeatures glad_vkGetPhysicalDeviceFeatures = glad_on_demand_impl_vkGetPhysicalDeviceFeatures;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 * pFeatures) {
    glad_vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2) glad_vk_on_demand_loader("vkGetPhysicalDeviceFeatures2");
    glad_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}
PFN_vkGetPhysicalDeviceFeatures2 glad_vkGetPhysicalDeviceFeatures2 = glad_on_demand_impl_vkGetPhysicalDeviceFeatures2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 * pFeatures) {
    glad_vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceFeatures2KHR");
    glad_vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
}
PFN_vkGetPhysicalDeviceFeatures2KHR glad_vkGetPhysicalDeviceFeatures2KHR = glad_on_demand_impl_vkGetPhysicalDeviceFeatures2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties * pFormatProperties) {
    glad_vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceFormatProperties");
    glad_vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}
PFN_vkGetPhysicalDeviceFormatProperties glad_vkGetPhysicalDeviceFormatProperties = glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 * pFormatProperties) {
    glad_vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceFormatProperties2");
    glad_vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, pFormatProperties);
}
PFN_vkGetPhysicalDeviceFormatProperties2 glad_vkGetPhysicalDeviceFormatProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 * pFormatProperties) {
    glad_vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceFormatProperties2KHR");
    glad_vkGetPhysicalDeviceFormatProperties2KHR(physicalDevice, format, pFormatProperties);
}
PFN_vkGetPhysicalDeviceFormatProperties2KHR glad_vkGetPhysicalDeviceFormatProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceFormatProperties2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties * pImageFormatProperties) {
    glad_vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceImageFormatProperties");
    return glad_vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}
PFN_vkGetPhysicalDeviceImageFormatProperties glad_vkGetPhysicalDeviceImageFormatProperties = glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 * pImageFormatInfo, VkImageFormatProperties2 * pImageFormatProperties) {
    glad_vkGetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceImageFormatProperties2");
    return glad_vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
PFN_vkGetPhysicalDeviceImageFormatProperties2 glad_vkGetPhysicalDeviceImageFormatProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 * pImageFormatInfo, VkImageFormatProperties2 * pImageFormatProperties) {
    glad_vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceImageFormatProperties2KHR");
    return glad_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
PFN_vkGetPhysicalDeviceImageFormatProperties2KHR glad_vkGetPhysicalDeviceImageFormatProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceImageFormatProperties2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties * pMemoryProperties) {
    glad_vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceMemoryProperties");
    glad_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}
PFN_vkGetPhysicalDeviceMemoryProperties glad_vkGetPhysicalDeviceMemoryProperties = glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 * pMemoryProperties) {
    glad_vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceMemoryProperties2");
    glad_vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}
PFN_vkGetPhysicalDeviceMemoryProperties2 glad_vkGetPhysicalDeviceMemoryProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 * pMemoryProperties) {
    glad_vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceMemoryProperties2KHR");
    glad_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
}
PFN_vkGetPhysicalDeviceMemoryProperties2KHR glad_vkGetPhysicalDeviceMemoryProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceMemoryProperties2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t * pRectCount, VkRect2D * pRects) {
    glad_vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR) glad_vk_on_demand_loader("vkGetPhysicalDevicePresentRectanglesKHR");
    return glad_vkGetPhysicalDevicePresentRectanglesKHR(physicalDevice, surface, pRectCount, pRects);
}
PFN_vkGetPhysicalDevicePresentRectanglesKHR glad_vkGetPhysicalDevicePresentRectanglesKHR = glad_on_demand_impl_vkGetPhysicalDevicePresentRectanglesKHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties * pProperties) {
    glad_vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceProperties");
    glad_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}
PFN_vkGetPhysicalDeviceProperties glad_vkGetPhysicalDeviceProperties = glad_on_demand_impl_vkGetPhysicalDeviceProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 * pProperties) {
    glad_vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceProperties2");
    glad_vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
}
PFN_vkGetPhysicalDeviceProperties2 glad_vkGetPhysicalDeviceProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceProperties2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 * pProperties) {
    glad_vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceProperties2KHR");
    glad_vkGetPhysicalDeviceProperties2KHR(physicalDevice, pProperties);
}
PFN_vkGetPhysicalDeviceProperties2KHR glad_vkGetPhysicalDeviceProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceProperties2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties * pQueueFamilyProperties) {
    glad_vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceQueueFamilyProperties");
    glad_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}
PFN_vkGetPhysicalDeviceQueueFamilyProperties glad_vkGetPhysicalDeviceQueueFamilyProperties = glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties2 * pQueueFamilyProperties) {
    glad_vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceQueueFamilyProperties2");
    glad_vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}
PFN_vkGetPhysicalDeviceQueueFamilyProperties2 glad_vkGetPhysicalDeviceQueueFamilyProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties2 * pQueueFamilyProperties) {
    glad_vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    glad_vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}
PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR glad_vkGetPhysicalDeviceQueueFamilyProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceQueueFamilyProperties2KHR;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t * pPropertyCount, VkSparseImageFormatProperties * pProperties) {
    glad_vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceSparseImageFormatProperties");
    glad_vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}
PFN_vkGetPhysicalDeviceSparseImageFormatProperties glad_vkGetPhysicalDeviceSparseImageFormatProperties = glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 * pFormatInfo, uint32_t * pPropertyCount, VkSparseImageFormatProperties2 * pProperties) {
    glad_vkGetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2) glad_vk_on_demand_loader("vkGetPhysicalDeviceSparseImageFormatProperties2");
    glad_vkGetPhysicalDeviceSparseImageFormatProperties2(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 glad_vkGetPhysicalDeviceSparseImageFormatProperties2 = glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties2;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 * pFormatInfo, uint32_t * pPropertyCount, VkSparseImageFormatProperties2 * pProperties) {
    glad_vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
    glad_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR glad_vkGetPhysicalDeviceSparseImageFormatProperties2KHR = glad_on_demand_impl_vkGetPhysicalDeviceSparseImageFormatProperties2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR * pSurfaceCapabilities) {
    glad_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    return glad_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR glad_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = glad_on_demand_impl_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t * pSurfaceFormatCount, VkSurfaceFormatKHR * pSurfaceFormats) {
    glad_vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceSurfaceFormatsKHR");
    return glad_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR glad_vkGetPhysicalDeviceSurfaceFormatsKHR = glad_on_demand_impl_vkGetPhysicalDeviceSurfaceFormatsKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t * pPresentModeCount, VkPresentModeKHR * pPresentModes) {
    glad_vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceSurfacePresentModesKHR");
    return glad_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR glad_vkGetPhysicalDeviceSurfacePresentModesKHR = glad_on_demand_impl_vkGetPhysicalDeviceSurfacePresentModesKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 * pSupported) {
    glad_vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceSurfaceSupportKHR");
    return glad_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}
PFN_vkGetPhysicalDeviceSurfaceSupportKHR glad_vkGetPhysicalDeviceSurfaceSupportKHR = glad_on_demand_impl_vkGetPhysicalDeviceSurfaceSupportKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t * pToolCount, VkPhysicalDeviceToolProperties * pToolProperties) {
    glad_vkGetPhysicalDeviceToolProperties = (PFN_vkGetPhysicalDeviceToolProperties) glad_vk_on_demand_loader("vkGetPhysicalDeviceToolProperties");
    return glad_vkGetPhysicalDeviceToolProperties(physicalDevice, pToolCount, pToolProperties);
}
PFN_vkGetPhysicalDeviceToolProperties glad_vkGetPhysicalDeviceToolProperties = glad_on_demand_impl_vkGetPhysicalDeviceToolProperties;
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
static VkBool32 GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display * display) {
    glad_vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    return glad_vkGetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
}
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR glad_vkGetPhysicalDeviceWaylandPresentationSupportKHR = glad_on_demand_impl_vkGetPhysicalDeviceWaylandPresentationSupportKHR;

#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
static VkBool32 GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
    glad_vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceWin32PresentationSupportKHR");
    return glad_vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
}
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR glad_vkGetPhysicalDeviceWin32PresentationSupportKHR = glad_on_demand_impl_vkGetPhysicalDeviceWin32PresentationSupportKHR;

#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
static VkBool32 GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t * connection, xcb_visualid_t visual_id) {
    glad_vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceXcbPresentationSupportKHR");
    return glad_vkGetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
}
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR glad_vkGetPhysicalDeviceXcbPresentationSupportKHR = glad_on_demand_impl_vkGetPhysicalDeviceXcbPresentationSupportKHR;

#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
static VkBool32 GLAD_API_PTR glad_on_demand_impl_vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display * dpy, VisualID visualID) {
    glad_vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR) glad_vk_on_demand_loader("vkGetPhysicalDeviceXlibPresentationSupportKHR");
    return glad_vkGetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
}
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR glad_vkGetPhysicalDeviceXlibPresentationSupportKHR = glad_on_demand_impl_vkGetPhysicalDeviceXlibPresentationSupportKHR;

#endif
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t * pDataSize, void * pData) {
    glad_vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData) glad_vk_on_demand_loader("vkGetPipelineCacheData");
    return glad_vkGetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}
PFN_vkGetPipelineCacheData glad_vkGetPipelineCacheData = glad_on_demand_impl_vkGetPipelineCacheData;
static void GLAD_API_PTR glad_on_demand_impl_vkGetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t * pData) {
    glad_vkGetPrivateData = (PFN_vkGetPrivateData) glad_vk_on_demand_loader("vkGetPrivateData");
    glad_vkGetPrivateData(device, objectType, objectHandle, privateDataSlot, pData);
}
PFN_vkGetPrivateData glad_vkGetPrivateData = glad_on_demand_impl_vkGetPrivateData;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void * pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    glad_vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults) glad_vk_on_demand_loader("vkGetQueryPoolResults");
    return glad_vkGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}
PFN_vkGetQueryPoolResults glad_vkGetQueryPoolResults = glad_on_demand_impl_vkGetQueryPoolResults;
static void GLAD_API_PTR glad_on_demand_impl_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D * pGranularity) {
    glad_vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity) glad_vk_on_demand_loader("vkGetRenderAreaGranularity");
    glad_vkGetRenderAreaGranularity(device, renderPass, pGranularity);
}
PFN_vkGetRenderAreaGranularity glad_vkGetRenderAreaGranularity = glad_on_demand_impl_vkGetRenderAreaGranularity;
static void GLAD_API_PTR glad_on_demand_impl_vkGetRenderingAreaGranularityKHR(VkDevice device, const VkRenderingAreaInfo * pRenderingAreaInfo, VkExtent2D * pGranularity) {
    glad_vkGetRenderingAreaGranularityKHR = (PFN_vkGetRenderingAreaGranularityKHR) glad_vk_on_demand_loader("vkGetRenderingAreaGranularityKHR");
    glad_vkGetRenderingAreaGranularityKHR(device, pRenderingAreaInfo, pGranularity);
}
PFN_vkGetRenderingAreaGranularityKHR glad_vkGetRenderingAreaGranularityKHR = glad_on_demand_impl_vkGetRenderingAreaGranularityKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t * pValue) {
    glad_vkGetSemaphoreCounterValue = (PFN_vkGetSemaphoreCounterValue) glad_vk_on_demand_loader("vkGetSemaphoreCounterValue");
    return glad_vkGetSemaphoreCounterValue(device, semaphore, pValue);
}
PFN_vkGetSemaphoreCounterValue glad_vkGetSemaphoreCounterValue = glad_on_demand_impl_vkGetSemaphoreCounterValue;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t * pSwapchainImageCount, VkImage * pSwapchainImages) {
    glad_vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) glad_vk_on_demand_loader("vkGetSwapchainImagesKHR");
    return glad_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}
PFN_vkGetSwapchainImagesKHR glad_vkGetSwapchainImagesKHR = glad_on_demand_impl_vkGetSwapchainImagesKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges) {
    glad_vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges) glad_vk_on_demand_loader("vkInvalidateMappedMemoryRanges");
    return glad_vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}
PFN_vkInvalidateMappedMemoryRanges glad_vkInvalidateMappedMemoryRanges = glad_on_demand_impl_vkInvalidateMappedMemoryRanges;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData) {
    glad_vkMapMemory = (PFN_vkMapMemory) glad_vk_on_demand_loader("vkMapMemory");
    return glad_vkMapMemory(device, memory, offset, size, flags, ppData);
}
PFN_vkMapMemory glad_vkMapMemory = glad_on_demand_impl_vkMapMemory;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache * pSrcCaches) {
    glad_vkMergePipelineCaches = (PFN_vkMergePipelineCaches) glad_vk_on_demand_loader("vkMergePipelineCaches");
    return glad_vkMergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}
PFN_vkMergePipelineCaches glad_vkMergePipelineCaches = glad_on_demand_impl_vkMergePipelineCaches;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo * pBindInfo, VkFence fence) {
    glad_vkQueueBindSparse = (PFN_vkQueueBindSparse) glad_vk_on_demand_loader("vkQueueBindSparse");
    return glad_vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}
PFN_vkQueueBindSparse glad_vkQueueBindSparse = glad_on_demand_impl_vkQueueBindSparse;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR * pPresentInfo) {
    glad_vkQueuePresentKHR = (PFN_vkQueuePresentKHR) glad_vk_on_demand_loader("vkQueuePresentKHR");
    return glad_vkQueuePresentKHR(queue, pPresentInfo);
}
PFN_vkQueuePresentKHR glad_vkQueuePresentKHR = glad_on_demand_impl_vkQueuePresentKHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo * pSubmits, VkFence fence) {
    glad_vkQueueSubmit = (PFN_vkQueueSubmit) glad_vk_on_demand_loader("vkQueueSubmit");
    return glad_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}
PFN_vkQueueSubmit glad_vkQueueSubmit = glad_on_demand_impl_vkQueueSubmit;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueueSubmit2(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 * pSubmits, VkFence fence) {
    glad_vkQueueSubmit2 = (PFN_vkQueueSubmit2) glad_vk_on_demand_loader("vkQueueSubmit2");
    return glad_vkQueueSubmit2(queue, submitCount, pSubmits, fence);
}
PFN_vkQueueSubmit2 glad_vkQueueSubmit2 = glad_on_demand_impl_vkQueueSubmit2;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 * pSubmits, VkFence fence) {
    glad_vkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR) glad_vk_on_demand_loader("vkQueueSubmit2KHR");
    return glad_vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence);
}
PFN_vkQueueSubmit2KHR glad_vkQueueSubmit2KHR = glad_on_demand_impl_vkQueueSubmit2KHR;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkQueueWaitIdle(VkQueue queue) {
    glad_vkQueueWaitIdle = (PFN_vkQueueWaitIdle) glad_vk_on_demand_loader("vkQueueWaitIdle");
    return glad_vkQueueWaitIdle(queue);
}
PFN_vkQueueWaitIdle glad_vkQueueWaitIdle = glad_on_demand_impl_vkQueueWaitIdle;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    glad_vkResetCommandBuffer = (PFN_vkResetCommandBuffer) glad_vk_on_demand_loader("vkResetCommandBuffer");
    return glad_vkResetCommandBuffer(commandBuffer, flags);
}
PFN_vkResetCommandBuffer glad_vkResetCommandBuffer = glad_on_demand_impl_vkResetCommandBuffer;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    glad_vkResetCommandPool = (PFN_vkResetCommandPool) glad_vk_on_demand_loader("vkResetCommandPool");
    return glad_vkResetCommandPool(device, commandPool, flags);
}
PFN_vkResetCommandPool glad_vkResetCommandPool = glad_on_demand_impl_vkResetCommandPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    glad_vkResetDescriptorPool = (PFN_vkResetDescriptorPool) glad_vk_on_demand_loader("vkResetDescriptorPool");
    return glad_vkResetDescriptorPool(device, descriptorPool, flags);
}
PFN_vkResetDescriptorPool glad_vkResetDescriptorPool = glad_on_demand_impl_vkResetDescriptorPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkResetEvent(VkDevice device, VkEvent event) {
    glad_vkResetEvent = (PFN_vkResetEvent) glad_vk_on_demand_loader("vkResetEvent");
    return glad_vkResetEvent(device, event);
}
PFN_vkResetEvent glad_vkResetEvent = glad_on_demand_impl_vkResetEvent;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence * pFences) {
    glad_vkResetFences = (PFN_vkResetFences) glad_vk_on_demand_loader("vkResetFences");
    return glad_vkResetFences(device, fenceCount, pFences);
}
PFN_vkResetFences glad_vkResetFences = glad_on_demand_impl_vkResetFences;
static void GLAD_API_PTR glad_on_demand_impl_vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    glad_vkResetQueryPool = (PFN_vkResetQueryPool) glad_vk_on_demand_loader("vkResetQueryPool");
    glad_vkResetQueryPool(device, queryPool, firstQuery, queryCount);
}
PFN_vkResetQueryPool glad_vkResetQueryPool = glad_on_demand_impl_vkResetQueryPool;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkSetEvent(VkDevice device, VkEvent event) {
    glad_vkSetEvent = (PFN_vkSetEvent) glad_vk_on_demand_loader("vkSetEvent");
    return glad_vkSetEvent(device, event);
}
PFN_vkSetEvent glad_vkSetEvent = glad_on_demand_impl_vkSetEvent;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkSetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data) {
    glad_vkSetPrivateData = (PFN_vkSetPrivateData) glad_vk_on_demand_loader("vkSetPrivateData");
    return glad_vkSetPrivateData(device, objectType, objectHandle, privateDataSlot, data);
}
PFN_vkSetPrivateData glad_vkSetPrivateData = glad_on_demand_impl_vkSetPrivateData;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo * pSignalInfo) {
    glad_vkSignalSemaphore = (PFN_vkSignalSemaphore) glad_vk_on_demand_loader("vkSignalSemaphore");
    return glad_vkSignalSemaphore(device, pSignalInfo);
}
PFN_vkSignalSemaphore glad_vkSignalSemaphore = glad_on_demand_impl_vkSignalSemaphore;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkTransitionImageLayoutEXT(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfo * pTransitions) {
    glad_vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT) glad_vk_on_demand_loader("vkTransitionImageLayoutEXT");
    return glad_vkTransitionImageLayoutEXT(device, transitionCount, pTransitions);
}
PFN_vkTransitionImageLayoutEXT glad_vkTransitionImageLayoutEXT = glad_on_demand_impl_vkTransitionImageLayoutEXT;
static void GLAD_API_PTR glad_on_demand_impl_vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags) {
    glad_vkTrimCommandPool = (PFN_vkTrimCommandPool) glad_vk_on_demand_loader("vkTrimCommandPool");
    glad_vkTrimCommandPool(device, commandPool, flags);
}
PFN_vkTrimCommandPool glad_vkTrimCommandPool = glad_on_demand_impl_vkTrimCommandPool;
static void GLAD_API_PTR glad_on_demand_impl_vkUnmapMemory(VkDevice device, VkDeviceMemory memory) {
    glad_vkUnmapMemory = (PFN_vkUnmapMemory) glad_vk_on_demand_loader("vkUnmapMemory");
    glad_vkUnmapMemory(device, memory);
}
PFN_vkUnmapMemory glad_vkUnmapMemory = glad_on_demand_impl_vkUnmapMemory;
static void GLAD_API_PTR glad_on_demand_impl_vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void * pData) {
    glad_vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate) glad_vk_on_demand_loader("vkUpdateDescriptorSetWithTemplate");
    glad_vkUpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate, pData);
}
PFN_vkUpdateDescriptorSetWithTemplate glad_vkUpdateDescriptorSetWithTemplate = glad_on_demand_impl_vkUpdateDescriptorSetWithTemplate;
static void GLAD_API_PTR glad_on_demand_impl_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet * pDescriptorCopies) {
    glad_vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets) glad_vk_on_demand_loader("vkUpdateDescriptorSets");
    glad_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}
PFN_vkUpdateDescriptorSets glad_vkUpdateDescriptorSets = glad_on_demand_impl_vkUpdateDescriptorSets;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence * pFences, VkBool32 waitAll, uint64_t timeout) {
    glad_vkWaitForFences = (PFN_vkWaitForFences) glad_vk_on_demand_loader("vkWaitForFences");
    return glad_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}
PFN_vkWaitForFences glad_vkWaitForFences = glad_on_demand_impl_vkWaitForFences;
static VkResult GLAD_API_PTR glad_on_demand_impl_vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo * pWaitInfo, uint64_t timeout) {
    glad_vkWaitSemaphores = (PFN_vkWaitSemaphores) glad_vk_on_demand_loader("vkWaitSemaphores");
    return glad_vkWaitSemaphores(device, pWaitInfo, timeout);
}
PFN_vkWaitSemaphores glad_vkWaitSemaphores = glad_on_demand_impl_vkWaitSemaphores;


 

#ifdef GLAD_VULKAN

#ifndef GLAD_LOADER_LIBRARY_C_
#define GLAD_LOADER_LIBRARY_C_

#include <stddef.h>
#include <stdlib.h>

#if GLAD_PLATFORM_WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


static void* glad_get_dlopen_handle(const char *lib_names[], int length) {
    void *handle = NULL;
    int i;

    for (i = 0; i < length; ++i) {
#if GLAD_PLATFORM_WIN32
  #if GLAD_PLATFORM_UWP
        size_t buffer_size = (strlen(lib_names[i]) + 1) * sizeof(WCHAR);
        LPWSTR buffer = (LPWSTR) malloc(buffer_size);
        if (buffer != NULL) {
            int ret = MultiByteToWideChar(CP_ACP, 0, lib_names[i], -1, buffer, buffer_size);
            if (ret != 0) {
                handle = (void*) LoadPackagedLibrary(buffer, 0);
            }
            free((void*) buffer);
        }
  #else
        handle = (void*) LoadLibraryA(lib_names[i]);
  #endif
#else
        handle = dlopen(lib_names[i], RTLD_LAZY | RTLD_LOCAL);
#endif
        if (handle != NULL) {
            return handle;
        }
    }

    return NULL;
}

static void glad_close_dlopen_handle(void* handle) {
    if (handle != NULL) {
#if GLAD_PLATFORM_WIN32
        FreeLibrary((HMODULE) handle);
#else
        dlclose(handle);
#endif
    }
}

static GLADapiproc glad_dlsym_handle(void* handle, const char *name) {
    if (handle == NULL) {
        return NULL;
    }

#if GLAD_PLATFORM_WIN32
    return (GLADapiproc) GetProcAddress((HMODULE) handle, name);
#else
    return GLAD_GNUC_EXTENSION (GLADapiproc) dlsym(handle, name);
#endif
}

#endif /* GLAD_LOADER_LIBRARY_C_ */


static const char* DEVICE_FUNCTIONS[] = {
    "vkAcquireNextImage2KHR",
    "vkAcquireNextImageKHR",
    "vkAllocateCommandBuffers",
    "vkAllocateDescriptorSets",
    "vkAllocateMemory",
    "vkBeginCommandBuffer",
    "vkBindBufferMemory",
    "vkBindBufferMemory2",
    "vkBindBufferMemory2KHR",
    "vkBindImageMemory",
    "vkBindImageMemory2",
    "vkBindImageMemory2KHR",
    "vkCmdBeginQuery",
    "vkCmdBeginRenderPass",
    "vkCmdBeginRenderPass2",
    "vkCmdBeginRenderPass2KHR",
    "vkCmdBeginRendering",
    "vkCmdBeginRenderingKHR",
    "vkCmdBindDescriptorBufferEmbeddedSamplers2EXT",
    "vkCmdBindDescriptorSets",
    "vkCmdBindDescriptorSets2KHR",
    "vkCmdBindIndexBuffer",
    "vkCmdBindIndexBuffer2KHR",
    "vkCmdBindPipeline",
    "vkCmdBindVertexBuffers",
    "vkCmdBindVertexBuffers2",
    "vkCmdBindVertexBuffers2EXT",
    "vkCmdBlitImage",
    "vkCmdBlitImage2",
    "vkCmdBlitImage2KHR",
    "vkCmdClearAttachments",
    "vkCmdClearColorImage",
    "vkCmdClearDepthStencilImage",
    "vkCmdCopyBuffer",
    "vkCmdCopyBuffer2",
    "vkCmdCopyBuffer2KHR",
    "vkCmdCopyBufferToImage",
    "vkCmdCopyBufferToImage2",
    "vkCmdCopyBufferToImage2KHR",
    "vkCmdCopyImage",
    "vkCmdCopyImage2",
    "vkCmdCopyImage2KHR",
    "vkCmdCopyImageToBuffer",
    "vkCmdCopyImageToBuffer2",
    "vkCmdCopyImageToBuffer2KHR",
    "vkCmdCopyQueryPoolResults",
    "vkCmdDispatch",
    "vkCmdDispatchBase",
    "vkCmdDispatchBaseKHR",
    "vkCmdDispatchIndirect",
    "vkCmdDraw",
    "vkCmdDrawIndexed",
    "vkCmdDrawIndexedIndirect",
    "vkCmdDrawIndexedIndirectCount",
    "vkCmdDrawIndexedIndirectCountKHR",
    "vkCmdDrawIndirect",
    "vkCmdDrawIndirectCount",
    "vkCmdDrawIndirectCountKHR",
    "vkCmdEndQuery",
    "vkCmdEndRenderPass",
    "vkCmdEndRenderPass2",
    "vkCmdEndRenderPass2KHR",
    "vkCmdEndRendering",
    "vkCmdEndRenderingKHR",
    "vkCmdExecuteCommands",
    "vkCmdFillBuffer",
    "vkCmdNextSubpass",
    "vkCmdNextSubpass2",
    "vkCmdNextSubpass2KHR",
    "vkCmdPipelineBarrier",
    "vkCmdPipelineBarrier2",
    "vkCmdPipelineBarrier2KHR",
    "vkCmdPushConstants",
    "vkCmdPushConstants2KHR",
    "vkCmdPushDescriptorSet2KHR",
    "vkCmdPushDescriptorSetKHR",
    "vkCmdPushDescriptorSetWithTemplate2KHR",
    "vkCmdPushDescriptorSetWithTemplateKHR",
    "vkCmdResetEvent",
    "vkCmdResetEvent2",
    "vkCmdResetEvent2KHR",
    "vkCmdResetQueryPool",
    "vkCmdResolveImage",
    "vkCmdResolveImage2",
    "vkCmdResolveImage2KHR",
    "vkCmdSetBlendConstants",
    "vkCmdSetCullMode",
    "vkCmdSetCullModeEXT",
    "vkCmdSetDepthBias",
    "vkCmdSetDepthBiasEnable",
    "vkCmdSetDepthBiasEnableEXT",
    "vkCmdSetDepthBounds",
    "vkCmdSetDepthBoundsTestEnable",
    "vkCmdSetDepthBoundsTestEnableEXT",
    "vkCmdSetDepthCompareOp",
    "vkCmdSetDepthCompareOpEXT",
    "vkCmdSetDepthTestEnable",
    "vkCmdSetDepthTestEnableEXT",
    "vkCmdSetDepthWriteEnable",
    "vkCmdSetDepthWriteEnableEXT",
    "vkCmdSetDescriptorBufferOffsets2EXT",
    "vkCmdSetDeviceMask",
    "vkCmdSetDeviceMaskKHR",
    "vkCmdSetEvent",
    "vkCmdSetEvent2",
    "vkCmdSetEvent2KHR",
    "vkCmdSetFrontFace",
    "vkCmdSetFrontFaceEXT",
    "vkCmdSetLineStippleEXT",
    "vkCmdSetLineWidth",
    "vkCmdSetLogicOpEXT",
    "vkCmdSetPatchControlPointsEXT",
    "vkCmdSetPrimitiveRestartEnable",
    "vkCmdSetPrimitiveRestartEnableEXT",
    "vkCmdSetPrimitiveTopology",
    "vkCmdSetPrimitiveTopologyEXT",
    "vkCmdSetRasterizerDiscardEnable",
    "vkCmdSetRasterizerDiscardEnableEXT",
    "vkCmdSetRenderingAttachmentLocationsKHR",
    "vkCmdSetRenderingInputAttachmentIndicesKHR",
    "vkCmdSetScissor",
    "vkCmdSetScissorWithCount",
    "vkCmdSetScissorWithCountEXT",
    "vkCmdSetStencilCompareMask",
    "vkCmdSetStencilOp",
    "vkCmdSetStencilOpEXT",
    "vkCmdSetStencilReference",
    "vkCmdSetStencilTestEnable",
    "vkCmdSetStencilTestEnableEXT",
    "vkCmdSetStencilWriteMask",
    "vkCmdSetVertexInputEXT",
    "vkCmdSetViewport",
    "vkCmdSetViewportWithCount",
    "vkCmdSetViewportWithCountEXT",
    "vkCmdUpdateBuffer",
    "vkCmdWaitEvents",
    "vkCmdWaitEvents2",
    "vkCmdWaitEvents2KHR",
    "vkCmdWriteTimestamp",
    "vkCmdWriteTimestamp2",
    "vkCmdWriteTimestamp2KHR",
    "vkCopyImageToImageEXT",
    "vkCopyImageToMemoryEXT",
    "vkCopyMemoryToImageEXT",
    "vkCreateBuffer",
    "vkCreateBufferView",
    "vkCreateCommandPool",
    "vkCreateComputePipelines",
    "vkCreateDescriptorPool",
    "vkCreateDescriptorSetLayout",
    "vkCreateDescriptorUpdateTemplate",
    "vkCreateEvent",
    "vkCreateFence",
    "vkCreateFramebuffer",
    "vkCreateGraphicsPipelines",
    "vkCreateImage",
    "vkCreateImageView",
    "vkCreatePipelineCache",
    "vkCreatePipelineLayout",
    "vkCreatePrivateDataSlot",
    "vkCreateQueryPool",
    "vkCreateRenderPass",
    "vkCreateRenderPass2",
    "vkCreateRenderPass2KHR",
    "vkCreateSampler",
    "vkCreateSamplerYcbcrConversion",
    "vkCreateSamplerYcbcrConversionKHR",
    "vkCreateSemaphore",
    "vkCreateShaderModule",
    "vkCreateSwapchainKHR",
    "vkDestroyBuffer",
    "vkDestroyBufferView",
    "vkDestroyCommandPool",
    "vkDestroyDescriptorPool",
    "vkDestroyDescriptorSetLayout",
    "vkDestroyDescriptorUpdateTemplate",
    "vkDestroyDevice",
    "vkDestroyEvent",
    "vkDestroyFence",
    "vkDestroyFramebuffer",
    "vkDestroyImage",
    "vkDestroyImageView",
    "vkDestroyPipeline",
    "vkDestroyPipelineCache",
    "vkDestroyPipelineLayout",
    "vkDestroyPrivateDataSlot",
    "vkDestroyQueryPool",
    "vkDestroyRenderPass",
    "vkDestroySampler",
    "vkDestroySamplerYcbcrConversion",
    "vkDestroySamplerYcbcrConversionKHR",
    "vkDestroySemaphore",
    "vkDestroyShaderModule",
    "vkDestroySwapchainKHR",
    "vkDeviceWaitIdle",
    "vkEndCommandBuffer",
    "vkFlushMappedMemoryRanges",
    "vkFreeCommandBuffers",
    "vkFreeDescriptorSets",
    "vkFreeMemory",
    "vkGetAndroidHardwareBufferPropertiesANDROID",
    "vkGetBufferDeviceAddress",
    "vkGetBufferDeviceAddressEXT",
    "vkGetBufferDeviceAddressKHR",
    "vkGetBufferMemoryRequirements",
    "vkGetBufferMemoryRequirements2",
    "vkGetBufferMemoryRequirements2KHR",
    "vkGetBufferOpaqueCaptureAddress",
    "vkGetBufferOpaqueCaptureAddressKHR",
    "vkGetDescriptorSetLayoutSupport",
    "vkGetDeviceBufferMemoryRequirements",
    "vkGetDeviceFaultInfoEXT",
    "vkGetDeviceGroupPeerMemoryFeatures",
    "vkGetDeviceGroupPeerMemoryFeaturesKHR",
    "vkGetDeviceGroupPresentCapabilitiesKHR",
    "vkGetDeviceGroupSurfacePresentModesKHR",
    "vkGetDeviceImageMemoryRequirements",
    "vkGetDeviceImageSparseMemoryRequirements",
    "vkGetDeviceImageSubresourceLayoutKHR",
    "vkGetDeviceMemoryCommitment",
    "vkGetDeviceMemoryOpaqueCaptureAddress",
    "vkGetDeviceMemoryOpaqueCaptureAddressKHR",
    "vkGetDeviceProcAddr",
    "vkGetDeviceQueue",
    "vkGetDeviceQueue2",
    "vkGetEventStatus",
    "vkGetFenceStatus",
    "vkGetImageMemoryRequirements",
    "vkGetImageMemoryRequirements2",
    "vkGetImageMemoryRequirements2KHR",
    "vkGetImageSparseMemoryRequirements",
    "vkGetImageSparseMemoryRequirements2",
    "vkGetImageSparseMemoryRequirements2KHR",
    "vkGetImageSubresourceLayout",
    "vkGetImageSubresourceLayout2EXT",
    "vkGetImageSubresourceLayout2KHR",
    "vkGetMemoryAndroidHardwareBufferANDROID",
    "vkGetPipelineCacheData",
    "vkGetPrivateData",
    "vkGetQueryPoolResults",
    "vkGetRenderAreaGranularity",
    "vkGetRenderingAreaGranularityKHR",
    "vkGetSemaphoreCounterValue",
    "vkGetSwapchainImagesKHR",
    "vkInvalidateMappedMemoryRanges",
    "vkMapMemory",
    "vkMergePipelineCaches",
    "vkQueueBindSparse",
    "vkQueuePresentKHR",
    "vkQueueSubmit",
    "vkQueueSubmit2",
    "vkQueueSubmit2KHR",
    "vkQueueWaitIdle",
    "vkResetCommandBuffer",
    "vkResetCommandPool",
    "vkResetDescriptorPool",
    "vkResetEvent",
    "vkResetFences",
    "vkResetQueryPool",
    "vkSetEvent",
    "vkSetPrivateData",
    "vkSignalSemaphore",
    "vkTransitionImageLayoutEXT",
    "vkTrimCommandPool",
    "vkUnmapMemory",
    "vkUpdateDescriptorSetWithTemplate",
    "vkUpdateDescriptorSets",
    "vkWaitForFences",
    "vkWaitSemaphores",
};

static int glad_vulkan_is_device_function(const char *name) {
    /* Exists as a workaround for:
     * https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2323
     *
     * `vkGetDeviceProcAddr` does not return NULL for non-device functions.
     */
    int i;
    int length = sizeof(DEVICE_FUNCTIONS) / sizeof(DEVICE_FUNCTIONS[0]);

    for (i=0; i < length; ++i) {
        if (strcmp(DEVICE_FUNCTIONS[i], name) == 0) {
            return 1;
        }
    }

    return 0;
}

struct _glad_vulkan_userptr {
    void *vk_handle;
    VkInstance vk_instance;
    VkDevice vk_device;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr;
};

static GLADapiproc glad_vulkan_get_proc(void *vuserptr, const char *name) {
    struct _glad_vulkan_userptr userptr = *(struct _glad_vulkan_userptr*) vuserptr;
    PFN_vkVoidFunction result = NULL;

    if (userptr.vk_device != NULL && glad_vulkan_is_device_function(name)) {
        result = userptr.get_device_proc_addr(userptr.vk_device, name);
    }

    if (result == NULL && userptr.vk_instance != NULL) {
        result = userptr.get_instance_proc_addr(userptr.vk_instance, name);
    }

    if(result == NULL) {
        result = (PFN_vkVoidFunction) glad_dlsym_handle(userptr.vk_handle, name);
    }

    return (GLADapiproc) result;
}


static void* _glad_Vulkan_loader_handle = NULL;

static void* glad_vulkan_dlopen_handle(void) {
    static const char *NAMES[] = {
#if GLAD_PLATFORM_APPLE
        "libvulkan.1.dylib",
#elif GLAD_PLATFORM_WIN32
        "vulkan-1.dll",
        "vulkan.dll",
#else
        "libvulkan.so.1",
        "libvulkan.so",
#endif
    };

    if (_glad_Vulkan_loader_handle == NULL) {
        _glad_Vulkan_loader_handle = glad_get_dlopen_handle(NAMES, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    return _glad_Vulkan_loader_handle;
}

static struct _glad_vulkan_userptr glad_vulkan_build_userptr(void *handle, VkInstance instance, VkDevice device) {
    struct _glad_vulkan_userptr userptr;
    userptr.vk_handle = handle;
    userptr.vk_instance = instance;
    userptr.vk_device = device;
    userptr.get_instance_proc_addr = (PFN_vkGetInstanceProcAddr) glad_dlsym_handle(handle, "vkGetInstanceProcAddr");
    userptr.get_device_proc_addr = (PFN_vkGetDeviceProcAddr) glad_dlsym_handle(handle, "vkGetDeviceProcAddr");
    return userptr;
}


#ifdef __cplusplus
static struct _glad_vulkan_userptr glad_vulkan_internal_loader_global_userptr = {};
#else
static struct _glad_vulkan_userptr glad_vulkan_internal_loader_global_userptr = { 0 };
#endif

void gladLoaderSetVulkanInstance(VkInstance instance) {
    glad_vulkan_internal_loader_global_userptr.vk_instance = instance;
}

void gladLoaderSetVulkanDevice(VkDevice device) {
    glad_vulkan_internal_loader_global_userptr.vk_device = device;
}

static GLADapiproc glad_vulkan_internal_loader_get_proc(const char *name) {
    if (glad_vulkan_internal_loader_global_userptr.vk_handle == NULL) {
        glad_vulkan_internal_loader_global_userptr = glad_vulkan_build_userptr(glad_vulkan_dlopen_handle(), NULL, NULL);
    }

    return glad_vulkan_get_proc((void *) &glad_vulkan_internal_loader_global_userptr, name);
}


void gladLoaderUnloadVulkan(void) {
    if (_glad_Vulkan_loader_handle != NULL) {
        glad_close_dlopen_handle(_glad_Vulkan_loader_handle);
        _glad_Vulkan_loader_handle = NULL;
        glad_vulkan_internal_loader_global_userptr.vk_handle = NULL;
    }
}

#endif /* GLAD_VULKAN */

#ifdef __cplusplus
}
#endif
