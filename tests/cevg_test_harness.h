/* =====================================================================
 * cevg_test_harness.h — Unified test harness for Cevg
 * ---------------------------------------------------------------------
 * Vulkan + Skia Graphite backend only (no backend selection needed).
 *
 * Usage:
 *   #include "cevg_test_harness.h"
 *
 *   CevgContext* ctx = cevg_test_init(800, 600);
 *   CevgSurface* surf = cevg_test_surface();
 *   CevgCanvas*  cv   = cevg_test_canvas();
 *
 *   // ... draw with cevg_canvas_* etc. ...
 *
 *   cevg_test_save_png("output.png");
 *   cevg_test_shutdown();
 * ===================================================================== */
#ifndef CEVG_TEST_HARNESS_H
#define CEVG_TEST_HARNESS_H

#include <cevg/cevg.h>
#include "test_util.h"

/* =====================================================================
 * Public API
 * ===================================================================== */

/* Initialize the Vulkan backend and create a Cevg GPU device +
 * context + surface + canvas of the given size. Returns NULL on failure. */
CevgContext*  cevg_test_init(int width, int height);

/* Access the created objects (valid after cevg_test_init). */
CevgContext*  cevg_test_ctx(void);
CevgSurface*  cevg_test_surface(void);
CevgCanvas*   cevg_test_canvas(void);

/* Read pixels from the surface and write to PNG.
 * Converts premultiplied alpha to straight alpha. */
void          cevg_test_save_png(const char* path);

/* Tear down everything (canvas, surface, context, GPU device, Vulkan). */
void          cevg_test_shutdown(void);

#endif /* CEVG_TEST_HARNESS_H */
