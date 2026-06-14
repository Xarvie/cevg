/* =====================================================================
 * vulkan_setup.c — Vulkan bootstrap for Cevg test harness
 * ---------------------------------------------------------------------
 * Headless mode: loads vulkan-1.dll dynamically, creates instance +
 * device with ALL device extensions (Skia Graphite requirement).
 *
 * This file is written as a REAL APP would: it does NOT use glad2.
 * It loads vkGetInstanceProcAddr from vulkan-1.dll, then uses raw
 * PFN_vk* function pointers for all Vulkan calls. The Vulkan handles
 * and vkGetInstanceProcAddr are then passed to cevg via
 * CevgVulkanDevice; cevg handles its own internal Vulkan function
 * loading via glad2.
 * ===================================================================== */
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

#include "vulkan_setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #define DLOPEN(lib)      LoadLibraryA(lib)
  #define DLSYM(h, name)   ((void*)GetProcAddress((HMODULE)(h), name))
  #define DLCLOSE(h)       FreeLibrary((HMODULE)(h))
  #define VK_LIB "vulkan-1.dll"
#else
  #include <dlfcn.h>
  #define DLOPEN(lib)      dlopen(lib, RTLD_NOW)
  #define DLSYM(h, name)   dlsym(h, name)
  #define DLCLOSE(h)       dlclose(h)
  #define VK_LIB "libvulkan.so.1"
#endif

struct CevgTestVK {
    void*             vk_dll;
    VkInstance        instance;
    VkPhysicalDevice  physical_device;
    VkDevice          device;
    VkQueue           queue;
    uint32_t          queue_family_index;
    /* Extension names — must stay alive for CevgVulkanDevice to reference */
    const char**      enabled_ext_names;
    uint32_t          enabled_ext_count;
    /* Stored extension property strings (owned) */
    VkExtensionProperties* ext_props;
    PFN_vkGetInstanceProcAddr get_proc_addr;
};

CevgTestVK* vk_test_init(void) {
    CevgTestVK* vk = (CevgTestVK*)calloc(1, sizeof(CevgTestVK));
    if (!vk) return NULL;

    /* 1. Load vkGetInstanceProcAddr from vulkan-1.dll */
    vk->vk_dll = DLOPEN(VK_LIB);
    if (!vk->vk_dll) { fprintf(stderr, "Cannot load %s\n", VK_LIB); free(vk); return NULL; }

    PFN_vkGetInstanceProcAddr gipa =
        (PFN_vkGetInstanceProcAddr)DLSYM(vk->vk_dll, "vkGetInstanceProcAddr");
    if (!gipa) { fprintf(stderr, "vkGetInstanceProcAddr not found\n"); DLCLOSE(vk->vk_dll); free(vk); return NULL; }
    vk->get_proc_addr = gipa;
    printf("  Vulkan: loaded %s\n", VK_LIB);

    /* 2. Create instance — load vkCreateInstance via gipa(NULL, ...) */
    PFN_vkCreateInstance vkCreateInstance =
        (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "CevgTest",
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 1, 0)
    };

    const char* instance_extensions[] = {
        "VK_KHR_surface",
#ifdef _WIN32
        "VK_KHR_win32_surface",
#endif
    };
    uint32_t instance_ext_count = sizeof(instance_extensions) / sizeof(instance_extensions[0]);

    VkInstanceCreateInfo instInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instance_ext_count,
        .ppEnabledExtensionNames = instance_extensions
    };
    if (vkCreateInstance(&instInfo, NULL, &vk->instance) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed\n"); DLCLOSE(vk->vk_dll); free(vk); return NULL;
    }
    printf("  Vulkan: instance created\n");

    /* 3. Enumerate physical devices */
    PFN_vkEnumeratePhysicalDevices vkEnumPhysDev =
        (PFN_vkEnumeratePhysicalDevices)gipa(vk->instance, "vkEnumeratePhysicalDevices");
    uint32_t dc = 0;
    vkEnumPhysDev(vk->instance, &dc, NULL);
    if (dc == 0) { fprintf(stderr, "No Vulkan devices\n"); vk_test_destroy(vk); return NULL; }
    VkPhysicalDevice* devs = (VkPhysicalDevice*)malloc(dc * sizeof(VkPhysicalDevice));
    vkEnumPhysDev(vk->instance, &dc, devs);
    vk->physical_device = devs[0];
    free(devs);

    /* 4. Find graphics queue family */
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetQueueProps =
        (PFN_vkGetPhysicalDeviceQueueFamilyProperties)gipa(vk->instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    uint32_t qfc = 0;
    vkGetQueueProps(vk->physical_device, &qfc, NULL);
    VkQueueFamilyProperties* qp = (VkQueueFamilyProperties*)malloc(qfc * sizeof(VkQueueFamilyProperties));
    vkGetQueueProps(vk->physical_device, &qfc, qp);
    vk->queue_family_index = 0;
    for (uint32_t i = 0; i < qfc; i++) {
        if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vk->queue_family_index = i;
            break;
        }
    }
    free(qp);

    /* 5. Enumerate and enable ALL device extensions (Skia Graphite requirement) */
    PFN_vkEnumerateDeviceExtensionProperties vkEnumDevExts =
        (PFN_vkEnumerateDeviceExtensionProperties)gipa(vk->instance, "vkEnumerateDeviceExtensionProperties");
    uint32_t ec = 0;
    vkEnumDevExts(vk->physical_device, NULL, &ec, NULL);
    vk->ext_props = (VkExtensionProperties*)malloc(ec * sizeof(VkExtensionProperties));
    vkEnumDevExts(vk->physical_device, NULL, &ec, vk->ext_props);
    vk->enabled_ext_names = (const char**)malloc(ec * sizeof(const char*));
    for (uint32_t i = 0; i < ec; i++)
        vk->enabled_ext_names[i] = vk->ext_props[i].extensionName;
    vk->enabled_ext_count = ec;

    /* 6. Create device */
    PFN_vkCreateDevice vkCreateDev =
        (PFN_vkCreateDevice)gipa(vk->instance, "vkCreateDevice");
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qi = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &priority
    };
    VkDeviceCreateInfo di = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qi,
        .enabledExtensionCount = ec,
        .ppEnabledExtensionNames = vk->enabled_ext_names
    };
    if (vkCreateDev(vk->physical_device, &di, NULL, &vk->device) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed\n"); vk_test_destroy(vk); return NULL;
    }
    printf("  Vulkan: device created (%u extensions)\n", ec);

    PFN_vkGetDeviceQueue vkGetDevQueue =
        (PFN_vkGetDeviceQueue)gipa(vk->instance, "vkGetDeviceQueue");
    vkGetDevQueue(vk->device, vk->queue_family_index, 0, &vk->queue);

    return vk;
}

/* ---- Accessors ---- */
void*    vk_test_instance(CevgTestVK* vk)         { return vk ? vk->instance : NULL; }
void*    vk_test_physical_device(CevgTestVK* vk)   { return vk ? (void*)vk->physical_device : NULL; }
void*    vk_test_device(CevgTestVK* vk)            { return vk ? vk->device : NULL; }
void*    vk_test_queue(CevgTestVK* vk)             { return vk ? vk->queue : NULL; }
uint32_t vk_test_queue_index(CevgTestVK* vk)       { return vk ? vk->queue_family_index : 0; }
const char** vk_test_enabled_ext_names(CevgTestVK* vk)  { return vk ? vk->enabled_ext_names : NULL; }
uint32_t     vk_test_enabled_ext_count(CevgTestVK* vk)  { return vk ? vk->enabled_ext_count : 0; }
void*    vk_test_get_proc_addr(CevgTestVK* vk)     { return vk ? (void*)vk->get_proc_addr : NULL; }

void vk_test_destroy(CevgTestVK* vk) {
    if (!vk) return;
    if (vk->device) {
        /* Load vkDeviceWaitIdle and vkDestroyDevice via gipa */
        PFN_vkDeviceWaitIdle vkWait =
            (PFN_vkDeviceWaitIdle)vk->get_proc_addr(vk->instance, "vkDeviceWaitIdle");
        PFN_vkDestroyDevice vkDestroyDev =
            (PFN_vkDestroyDevice)vk->get_proc_addr(vk->instance, "vkDestroyDevice");
        if (vkWait) vkWait(vk->device);
        if (vkDestroyDev) vkDestroyDev(vk->device, NULL);
    }
    if (vk->instance) {
        PFN_vkDestroyInstance vkDestroyInst =
            (PFN_vkDestroyInstance)vk->get_proc_addr(vk->instance, "vkDestroyInstance");
        if (vkDestroyInst) vkDestroyInst(vk->instance, NULL);
    }
    free(vk->enabled_ext_names);
    free(vk->ext_props);
    if (vk->vk_dll) DLCLOSE(vk->vk_dll);
    free(vk);
}
