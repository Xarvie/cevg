/* =====================================================================
 * cevg_test_harness.c — Cevg test harness implementation
 * ---------------------------------------------------------------------
 * Vulkan + Skia Graphite only. Uses cevg_gpu_device_create_vulkan()
 * and CevgVulkanDevice / CevgConfig with struct_size ABI.
 * ===================================================================== */
#include "cevg_test_harness.h"
#include "vulkan_setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Global state ---- */
static CevgGpuDevice* g_gpu  = NULL;
static CevgContext*   g_ctx  = NULL;
static CevgSurface*   g_surf = NULL;
static CevgCanvas*    g_cv   = NULL;
static CevgTestVK*    g_vk   = NULL;
static int            g_w    = 0;
static int            g_h    = 0;

CevgContext* cevg_test_init(int width, int height) {
    g_w = width;
    g_h = height;

    /* 1. Vulkan bootstrap */
    g_vk = vk_test_init();
    if (!g_vk) { fprintf(stderr, "cevg_test_init: Vulkan failed\n"); return NULL; }
    printf("[harness] Vulkan OK\n");

    /* 2. Create Cevg GPU device from Vulkan */
    CevgVulkanDevice vk_dev = {
        .struct_size            = sizeof(CevgVulkanDevice),
        .vk_instance            = vk_test_instance(g_vk),
        .vk_physical_device     = vk_test_physical_device(g_vk),
        .vk_device              = vk_test_device(g_vk),
        .vk_queue               = vk_test_queue(g_vk),
        .vk_queue_index         = vk_test_queue_index(g_vk),
        .enabled_ext_names      = vk_test_enabled_ext_names(g_vk),
        .enabled_ext_count      = vk_test_enabled_ext_count(g_vk),
        .vk_get_instance_proc_addr = vk_test_get_proc_addr(g_vk),
    };

    g_gpu = cevg_gpu_device_create_vulkan(&vk_dev);
    if (!g_gpu) { fprintf(stderr, "cevg_test_init: cevg_gpu_device_create_vulkan failed\n"); return NULL; }
    printf("[harness] GPU device OK\n");

    /* 3. Create Cevg context */
    CevgConfig cfg = {
        .struct_size  = sizeof(CevgConfig),
        .device       = g_gpu,
        .color_space  = kCevgColorSpace_sRGB,
        .sample_count = 0,
    };

    g_ctx = cevg_context_create(&cfg);
    if (!g_ctx) { fprintf(stderr, "cevg_test_init: cevg_context_create failed\n"); return NULL; }
    printf("[harness] Cevg context OK\n");

    /* 4. Surface + Canvas */
    g_surf = cevg_surface_create(g_ctx, width, height, NULL);
    if (!g_surf) { fprintf(stderr, "cevg_test_init: surface create failed\n"); return NULL; }

    g_cv = cevg_canvas_create(g_surf);
    if (!g_cv) { fprintf(stderr, "cevg_test_init: canvas create failed\n"); return NULL; }

    return g_ctx;
}

CevgContext* cevg_test_ctx(void)      { return g_ctx; }
CevgSurface* cevg_test_surface(void)  { return g_surf; }
CevgCanvas*  cevg_test_canvas(void)   { return g_cv; }

void cevg_test_save_png(const char* path) {
    if (!g_surf) return;
    /* Insert "_gpu" before .png extension */
    char out_path[1024];
    const char* dot = strrchr(path, '.');
    if (dot) {
        size_t base = (size_t)(dot - path);
        memcpy(out_path, path, base);
        memcpy(out_path + base, "_gpu", 4);
        strcpy(out_path + base + 4, dot);
    } else {
        snprintf(out_path, sizeof(out_path), "%s_gpu", path);
    }
    unsigned char* pixels = (unsigned char*)malloc(g_w * g_h * 4);
    if (!pixels) return;
    memset(pixels, 0, g_w * g_h * 4);
    CevgResult r = cevg_surface_read_pixels(g_surf, pixels);
    if (r != 0) fprintf(stderr, "[harness] read_pixels failed: %d\n", r);
    test_write_png(out_path, pixels, g_w, g_h);
    printf("PNG saved: %s (%dx%d)\n", out_path, g_w, g_h);
    free(pixels);
}

void cevg_test_shutdown(void) {
    if (g_cv)   cevg_canvas_destroy(g_cv);
    if (g_surf) cevg_surface_destroy(g_surf);
    if (g_ctx)  cevg_context_destroy(g_ctx);
    if (g_gpu)  cevg_gpu_device_destroy(g_gpu);
    if (g_vk)   vk_test_destroy(g_vk);
    g_cv = NULL; g_surf = NULL; g_ctx = NULL; g_gpu = NULL; g_vk = NULL;
}
