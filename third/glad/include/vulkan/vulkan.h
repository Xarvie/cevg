/* Hijack: redirect <vulkan/vulkan.h> to glad2's self-contained header.
   All Vulkan types, enums, and function-pointer declarations come from
   glad/vulkan.h.  The on-demand loader resolves symbols at runtime from
   vulkan-1.dll / libvulkan.so — no Vulkan SDK needed at compile time.
   No include-guard here: glad/vulkan.h defines VULKAN_H_ itself and has
   its own GLAD_VULKAN_H_ guard to prevent re-inclusion. */
#include <glad/vulkan.h>
