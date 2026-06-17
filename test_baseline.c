/* Verify whether cevg_canvas_draw_text_blob(x,y) treats y as baseline or top.
 *
 * Skia convention: y = baseline. If we draw at y=50 with a 28px font,
 * glyph pixels should appear around y=[50+ascent, 50+descent].
 * (ascent is negative, so 50+ascent < 50)
 * If cevg treats y as top, pixels would be at y=[50, 50+lineHeight].
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cevg/cevg.h"

int main() {
    CevgGpuDevice* dev = cevg_gpu_device_create_cpu();
    if (!dev) { fprintf(stderr, "FAIL: cpu device\n"); return 1; }

    CevgConfig cfg = {0};
    cfg.struct_size = sizeof(cfg);
    cfg.device = dev;
    cfg.color_space = kCevgColorSpace_sRGB;
    CevgContext* ctx = cevg_context_create(&cfg);
    if (!ctx) { fprintf(stderr, "FAIL: context\n"); return 1; }

    /* Create a pixel buffer for the surface */
    uint8_t pixels_buf[200 * 200 * 4];
    memset(pixels_buf, 0, sizeof(pixels_buf));

    CevgSurface* surf = cevg_surface_create(ctx, 200, 200, pixels_buf);
    if (!surf) { fprintf(stderr, "FAIL: surface\n"); return 1; }

    CevgCanvas* cv = cevg_canvas_create(surf);
    if (!cv) { fprintf(stderr, "FAIL: canvas\n"); return 1; }

    /* Clear to black */
    cevg_canvas_clear(cv, 0, 0, 0, 1);

    /* Load font */
    CevgTypeface* face = cevg_typeface_create_from_file(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);
    if (!face) {
        fprintf(stderr, "SKIP: DejaVuSans.ttf not found\n");
        cevg_canvas_destroy(cv);
        cevg_surface_destroy(surf);
        cevg_context_destroy(ctx);
        return 0;
    }

    /* Get font metrics to know ascent/descent */
    CevgFontMetrics metrics = {0};
    metrics.struct_size = sizeof(metrics);
    cevg_typeface_get_metrics(face, 28.0f, &metrics);
    float lineHeight = metrics.descent - metrics.ascent;
    fprintf(stderr, "[test] Font metrics: ascent=%.2f descent=%.2f lineHeight=%.2f\n",
            metrics.ascent, metrics.descent, lineHeight);

    /* Draw "Ag" at y=50.0 */
    CevgTextBlob* blob = cevg_text_blob_make("Ag", 2, face, 28.0f, kCevgDir_LTR);
    if (!blob) { fprintf(stderr, "FAIL: blob\n"); return 1; }

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    cevg_canvas_draw_text_blob(cv, blob, 10.0f, 50.0f, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
    cevg_typeface_unref(face);

    cevg_canvas_flush(cv);

    /* Read pixels */
    uint8_t out_buf[200 * 200 * 4];
    CevgResult res = cevg_surface_read_pixels(surf, out_buf);
    if (res != kCevgSuccess) { fprintf(stderr, "FAIL: read_pixels %d\n", res); return 1; }

    /* Find the topmost and bottommost non-black pixel */
    int top_y = 200, bot_y = 0;
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 200; x++) {
            int idx = (y * 200 + x) * 4;
            if (out_buf[idx] > 10 || out_buf[idx+1] > 10 || out_buf[idx+2] > 10) {
                if (y < top_y) top_y = y;
                if (y > bot_y) bot_y = y;
            }
        }
    }

    cevg_canvas_destroy(cv);
    cevg_surface_destroy(surf);
    cevg_context_destroy(ctx);
    cevg_gpu_device_destroy(dev);

    fprintf(stderr, "[test] Text pixel range: y=[%d, %d]\n", top_y, bot_y);
    fprintf(stderr, "[test] Draw y=50, ascent=%.2f, descent=%.2f, lineHeight=%.2f\n",
            metrics.ascent, metrics.descent, lineHeight);

    float baseline_top = 50.0f + metrics.ascent;  /* ascent is negative, so < 50 */
    float baseline_bot = 50.0f + metrics.descent;
    float top_top = 50.0f;
    float top_bot = 50.0f + lineHeight;

    fprintf(stderr, "[test] If BASELINE convention: expected y=[%.1f, %.1f]\n", baseline_top, baseline_bot);
    fprintf(stderr, "[test] If TOP convention:      expected y=[%.1f, %.1f]\n", top_top, top_bot);

    /* Check which convention matches — use bottom pixel as the primary
     * discriminator, since top pixel is affected by the gap between
     * cap-height and ascent (diacritics space). The descent line is
     * where the bottom of "g" should be, which is much tighter. */
    int baseline_bot_int = (int)(baseline_bot + 0.5f);
    int top_bot_int = (int)(top_bot + 0.5f);

    if (abs(bot_y - baseline_bot_int) <= 3) {
        fprintf(stderr, "[test] RESULT: BASELINE convention (Skia standard) — "
                "bot_y=%d ≈ baseline_bot=%d\n", bot_y, baseline_bot_int);
    } else if (abs(bot_y - top_bot_int) <= 3) {
        fprintf(stderr, "[test] RESULT: TOP convention (non-standard) — "
                "bot_y=%d ≈ top_bot=%d\n", bot_y, top_bot_int);
    } else {
        fprintf(stderr, "[test] RESULT: UNCLEAR — bot_y=%d, baseline_bot=%d, top_bot=%d\n",
                bot_y, baseline_bot_int, top_bot_int);
    }

    return 0;
}
