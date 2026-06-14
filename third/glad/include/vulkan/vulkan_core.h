/* Hijack: redirect <vulkan/vulkan_core.h> to glad2.
   All core Vulkan types are already defined by glad/vulkan.h.
   This stub satisfies #include <vulkan/vulkan_core.h> without
   pulling in the Khronos SDK headers.
   No include-guard here: glad/vulkan.h defines VULKAN_CORE_H_ itself
   and has its own GLAD_VULKAN_H_ guard to prevent re-inclusion. */
#include <glad/vulkan.h>

/* =======================================================================
   Compatibility layer — fills gaps in glad2 Vulkan 1.3 + extensions output
   that Skia m150 needs but glad2 does not emit:
     1. VK_API_VERSION_1_4
     2. VkPhysicalDeviceVulkan14Features (Vulkan 1.4 feature struct)
     3. Unsuffixed Vulkan 1.4 function-pointer aliases
   ======================================================================= */

/* ---- VK_API_VERSION_1_4 ---- */
#ifndef VK_API_VERSION_1_4
#define VK_API_VERSION_1_4 VK_MAKE_API_VERSION(0, 1, 4, 0)
#endif

/* ---- VkPhysicalDeviceVulkan14Features ---- */
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES ((VkStructureType)55)
#endif

#ifndef VkPhysicalDeviceVulkan14Features
typedef struct VkPhysicalDeviceVulkan14Features {
    VkStructureType sType;
    void*           pNext;
    VkBool32        globalPriorityQuery;
    VkBool32        shaderSubgroupRotate;
    VkBool32        shaderSubgroupRotateClustered;
    VkBool32        shaderFloatControls2;
    VkBool32        shaderExpectAssume;
    VkBool32        rectangularLines;
    VkBool32        bresenhamLines;
    VkBool32        smoothLines;
    VkBool32        stippledRectangularLines;
    VkBool32        stippledBresenhamLines;
    VkBool32        stippledSmoothLines;
    VkBool32        vertexAttributeInstanceRateDivisor;
    VkBool32        vertexAttributeInstanceRateZeroDivisor;
    VkBool32        indexTypeUint8;
    VkBool32        dynamicRenderingLocalRead;
    VkBool32        maintenance5;
    VkBool32        maintenance6;
    VkBool32        pipelineProtectedAccess;
    VkBool32        pipelineRobustness;
    VkBool32        hostImageCopy;
    VkBool32        pushDescriptor;
} VkPhysicalDeviceVulkan14Features;
#endif

/* ---- Vulkan 1.4 core function-pointer aliases (EXT → core) ---- */
#ifndef PFN_vkTransitionImageLayout
typedef PFN_vkTransitionImageLayoutEXT PFN_vkTransitionImageLayout;
#endif
#ifndef PFN_vkCopyMemoryToImage
typedef PFN_vkCopyMemoryToImageEXT PFN_vkCopyMemoryToImage;
#endif
#ifndef PFN_vkCopyImageToMemory
typedef PFN_vkCopyImageToMemoryEXT PFN_vkCopyImageToMemory;
#endif
