/* =====================================================================
 * cevg_test_harness_cpu.c — CPU backend test harness implementation
 * ---------------------------------------------------------------------
 * Drop-in replacement for cevg_test_harness.c that uses
 * cevg_gpu_device_create_cpu() instead of Vulkan.
 * No GPU / Vulkan / WSI required — runs on any machine.
 * ===================================================================== */
#include "cevg_test_harness.h"
#include <cevg/cevg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Global state ---- */
static CevgGpuDevice* g_gpu  = NULL;
static CevgContext*   g_ctx  = NULL;
static CevgSurface*   g_surf = NULL;
static CevgCanvas*    g_cv   = NULL;
static int            g_w    = 0;
static int            g_h    = 0;

CevgContext* cevg_test_init(int width, int height) {
    g_w = width;
    g_h = height;

    /* 1. Create CPU GPU device */
    g_gpu = cevg_gpu_device_create_cpu();
    if (!g_gpu) { fprintf(stderr, "cevg_test_init: CPU device create failed\n"); return NULL; }
    printf("[harness-cpu] CPU device OK\n");

    /* 2. Create Cevg context */
    CevgConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.struct_size  = sizeof(CevgConfig);
    cfg.device       = g_gpu;
    cfg.color_space  = kCevgColorSpace_sRGB;

    g_ctx = cevg_context_create(&cfg);
    if (!g_ctx) { fprintf(stderr, "cevg_test_init: context create failed\n"); return NULL; }
    printf("[harness-cpu] Cevg context OK\n");

    /* 3. Surface + Canvas */
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
    /* Insert "_cpu" before .png extension */
    char out_path[1024];
    const char* dot = strrchr(path, '.');
    if (dot) {
        size_t base = (size_t)(dot - path);
        memcpy(out_path, path, base);
        memcpy(out_path + base, "_cpu", 4);
        strcpy(out_path + base + 4, dot);
    } else {
        snprintf(out_path, sizeof(out_path), "%s_cpu", path);
    }
    unsigned char* pixels = (unsigned char*)malloc((size_t)g_w * g_h * 4);
    if (!pixels) return;
    memset(pixels, 0, (size_t)g_w * g_h * 4);
    CevgResult r = cevg_surface_read_pixels(g_surf, pixels);
    if (r != 0) fprintf(stderr, "[harness-cpu] read_pixels failed: %d\n", r);
    test_write_png(out_path, pixels, g_w, g_h);
    printf("PNG saved: %s (%dx%d)\n", out_path, g_w, g_h);
    free(pixels);
}

void cevg_test_shutdown(void) {
    if (g_cv)   cevg_canvas_destroy(g_cv);
    if (g_surf) cevg_surface_destroy(g_surf);
    if (g_ctx)  cevg_context_destroy(g_ctx);
    if (g_gpu)  cevg_gpu_device_destroy(g_gpu);
    g_cv = NULL; g_surf = NULL; g_ctx = NULL; g_gpu = NULL;
}
