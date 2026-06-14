/* =====================================================================
 * vulkan_setup.h — Vulkan bootstrap for Cevg test harness
 * ---------------------------------------------------------------------
 * Creates a VkInstance + VkDevice with all available device extensions
 * (required by Skia Graphite). Call vk_test_init() once, then use
 * the accessors to fill CevgVulkanDevice for cevg_gpu_device_create_vulkan().
 * ===================================================================== */
#ifndef CEVG_TEST_VULKAN_SETUP_H
#define CEVG_TEST_VULKAN_SETUP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CevgTestVK CevgTestVK;

/* Create a Vulkan instance + device with all device extensions enabled.
 * Returns NULL on failure. */
CevgTestVK* vk_test_init(void);

/* Accessors — return void* so they map directly to CevgVulkanDevice fields */
void*    vk_test_instance(CevgTestVK* vk);
void*    vk_test_physical_device(CevgTestVK* vk);
void*    vk_test_device(CevgTestVK* vk);
void*    vk_test_queue(CevgTestVK* vk);
uint32_t vk_test_queue_index(CevgTestVK* vk);

/* The enabled device extension names/count — pass to CevgVulkanDevice */
const char** vk_test_enabled_ext_names(CevgTestVK* vk);
uint32_t     vk_test_enabled_ext_count(CevgTestVK* vk);

/* vkGetInstanceProcAddr — pass to CevgVulkanDevice */
void*    vk_test_get_proc_addr(CevgTestVK* vk);

/* Tear down Vulkan resources. */
void vk_test_destroy(CevgTestVK* vk);

#ifdef __cplusplus
}
#endif

#endif /* CEVG_TEST_VULKAN_SETUP_H */
