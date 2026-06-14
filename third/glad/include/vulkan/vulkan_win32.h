/* Hijack: redirect <vulkan/vulkan_win32.h> to glad2.
   Win32 surface extensions (VK_KHR_win32_surface) are already defined
   in glad/vulkan.h (generated with that extension enabled).
   No include-guard here: glad/vulkan.h defines VULKAN_CORE_H_ itself
   and has its own GLAD_VULKAN_H_ guard to prevent re-inclusion. */
#include <glad/vulkan.h>
