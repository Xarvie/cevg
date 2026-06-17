/* =====================================================================
 * test_text_measure.c — Visual verification of text measurement fixes
 * ---------------------------------------------------------------------
 * Validates fixes for issues ①②③④:
 *   ① get_width uses pen-advance (not conservative bounds)
 *   ② Per-glyph advances from SkFont::getWidths (not position diffs)
 *   ③ Last glyph advance is correct (not inflated)
 *   ④ hit_test is advance-based (not nearest-origin)
 *
 * Generates a PNG image with visual annotations for manual inspection.
 * ===================================================================== */
#include "cevg_test_harness.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Colors ---- */
#define COL_WHITE   0xFFFFFFFF
#define COL_BLACK   0xFF000000
#define COL_RED     0xFFFF3333
#define COL_GREEN   0xFF33CC33
#define COL_BLUE    0xFF3399FF
#define COL_YELLOW  0xFFFFFF33
#define COL_ORANGE  0xFFFF9933
#define COL_CYAN    0xFF33FFFF
#define COL_GRAY    0xFF999999
#define COL_DKGRAY  0xFF444444
#define COL_LTGRAY  0xFFCCCCCC

static void set_color(CevgPaint* p, uint32_t argb) {
    cevg_paint_set_color_argb(p, argb);
}

/* Draw a vertical line (dashed if dash_len > 0) */
static void draw_vline(CevgCanvas* cv, float x, float y0, float y1,
                       CevgPaint* p, float dash_len) {
    if (dash_len <= 0) {
        cevg_canvas_draw_line(cv, x, y0, x, y1, p);
    } else {
        float y = y0;
        while (y < y1) {
            float yend = y + dash_len;
            if (yend > y1) yend = y1;
            cevg_canvas_draw_line(cv, x, y, x, yend, p);
            y = yend + dash_len;
        }
    }
}

/* Draw a horizontal line */
static void draw_hline(CevgCanvas* cv, float x0, float x1, float y,
                       CevgPaint* p) {
    cevg_canvas_draw_line(cv, x0, y, x1, y, p);
}

/* Draw a small label using built-in text (we draw a simple marker instead
 * since we can't render arbitrary text annotations without a font) */
static void draw_marker(CevgCanvas* cv, float x, float y, float size,
                        CevgPaint* p) {
    cevg_canvas_draw_line(cv, x - size, y - size, x + size, y + size, p);
    cevg_canvas_draw_line(cv, x - size, y + size, x + size, y - size, p);
}

/* =====================================================================
 * Test 1: Width vs Ink Bounds
 * ---------------------------------------------------------------------
 * Shape "MMM" and verify:
 *   - get_width() < ink_bounds.right (pen-advance < conservative box)
 *   - get_width() is close to sum of advances
 * ===================================================================== */
static int test_width_vs_ink(CevgCanvas* cv, CevgTypeface* face,
                             float origin_x, float origin_y) {
    CevgPaint* white  = cevg_paint_create(); set_color(white, COL_WHITE);
    CevgPaint* red    = cevg_paint_create(); set_color(red, COL_RED);
    CevgPaint* green  = cevg_paint_create(); set_color(green, COL_GREEN);
    CevgPaint* blue   = cevg_paint_create(); set_color(blue, COL_BLUE);
    CevgPaint* cyan   = cevg_paint_create(); set_color(cyan, COL_CYAN);
    CevgPaint* orange = cevg_paint_create(); set_color(orange, COL_ORANGE);
    CevgPaint* gray   = cevg_paint_create(); set_color(gray, COL_GRAY);
    CevgPaint* ltgray = cevg_paint_create(); set_color(ltgray, COL_LTGRAY);

    int pass = 1;
    const char* text = "MMM";
    size_t len = strlen(text);
    float fontSize = 64.0f;

    CevgTextBlob* blob = cevg_text_blob_make(text, len, face, fontSize, kCevgDir_LTR);
    if (!blob) { fprintf(stderr, "SKIP: no blob\n"); return 0; }

    float width = cevg_text_blob_get_width(blob);
    float ink[4];
    cevg_text_blob_get_ink_bounds(blob, ink);
    int glyph_count = cevg_text_blob_get_glyph_count(blob);

    /* Get advances and positions */
    float* advances = (float*)malloc(glyph_count * sizeof(float));
    float* pos_x = (float*)malloc(glyph_count * sizeof(float));
    float* pos_y = (float*)malloc(glyph_count * sizeof(float));
    cevg_text_blob_get_glyph_advances(blob, advances);
    cevg_text_blob_get_glyph_positions(blob, pos_x, pos_y);

    float sum_advances = 0;
    for (int i = 0; i < glyph_count; i++) sum_advances += advances[i];

    /* ---- Draw the text ---- */
    float baseline_y = origin_y;
    cevg_canvas_draw_text_blob(cv, blob, origin_x, baseline_y, white);

    /* ---- Draw baseline ---- */
    cevg_paint_set_stroke_width(gray, 1.0f);
    cevg_paint_set_style(gray, kCevgStyle_Stroke);
    draw_hline(cv, origin_x - 20, origin_x + width + 40, baseline_y, gray);

    /* ---- Draw pen-advance width line (green) ---- */
    cevg_paint_set_stroke_width(green, 2.0f);
    draw_vline(cv, origin_x + width, baseline_y - 80, baseline_y + 20, green, 0);

    /* ---- Draw ink bounds right edge (red, dashed) ---- */
    float ink_right = ink[0] + ink[2];  /* left + width */
    cevg_paint_set_stroke_width(red, 2.0f);
    draw_vline(cv, origin_x + ink_right, baseline_y - 80, baseline_y + 20, red, 8);

    /* ---- Draw ink bounds box (red, dashed outline) ---- */
    cevg_paint_set_style(red, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(red, 1.0f);
    cevg_canvas_draw_rect(cv, origin_x + ink[0], baseline_y + ink[1],
                          ink[2], ink[3], red);

    /* ---- Draw per-glyph advance boundaries (blue, dashed) ---- */
    cevg_paint_set_stroke_width(blue, 1.0f);
    for (int i = 0; i < glyph_count; i++) {
        float glyph_right = pos_x[i] + advances[i];
        float abs_right = origin_x + glyph_right;
        draw_vline(cv, abs_right, baseline_y - 50, baseline_y + 10, blue, 6);
    }

    /* ---- Draw glyph origin markers (cyan) ---- */
    for (int i = 0; i < glyph_count; i++) {
        draw_marker(cv, origin_x + pos_x[i], baseline_y, 4, cyan);
    }

    /* ---- Console output ---- */
    printf("\n=== Test 1: Width vs Ink Bounds (\"MMM\" at %.0fpx) ===\n", fontSize);
    printf("  get_width (pen-advance):  %.2f\n", width);
    printf("  ink_bounds: left=%.2f top=%.2f w=%.2f h=%.2f (right=%.2f)\n",
           ink[0], ink[1], ink[2], ink[3], ink_right);
    printf("  sum of advances:          %.2f\n", sum_advances);
    printf("  glyph count:              %d\n", glyph_count);
    printf("  NOTE: ink_bounds is CONSERVATIVE (uses font global extremes,\n");
    printf("        not actual M ink). Left gap=%.2f, right gap=%.2f — asymmetry\n",
           -ink[0], ink_right - width);
    printf("        is expected because the font's max left side bearing\n");
    printf("        differs from its max right extent.\n");
    for (int i = 0; i < glyph_count; i++) {
        printf("    glyph[%d]: pos_x=%.2f  advance=%.2f  right_edge=%.2f\n",
               i, pos_x[i], advances[i], pos_x[i] + advances[i]);
    }

    /* ---- Assertions ---- */
    /* ① width should be LESS than ink_bounds right edge (pen-advance < conservative box) */
    if (width >= ink_right) {
        printf("  FAIL ①: width (%.2f) should be < ink_right (%.2f)\n", width, ink_right);
        pass = 0;
    } else {
        printf("  PASS ①: width (%.2f) < ink_right (%.2f) — pen-advance is tighter than conservative box\n",
               width, ink_right);
    }

    /* ② width should be close to sum of advances */
    float diff = fabsf(width - sum_advances);
    if (diff > 2.0f) {
        printf("  FAIL ②: width (%.2f) differs from sum_advances (%.2f) by %.2f\n",
               width, sum_advances, diff);
        pass = 0;
    } else {
        printf("  PASS ②: width (%.2f) ≈ sum_advances (%.2f) — diff=%.2f\n",
               width, sum_advances, diff);
    }

    /* ③ All M advances should be identical (same glyph, no kerning between same chars) */
    int adv_consistent = 1;
    for (int i = 1; i < glyph_count; i++) {
        if (fabsf(advances[i] - advances[0]) > 0.5f) {
            printf("  FAIL ③: advance[%d]=%.2f differs from advance[0]=%.2f\n",
                   i, advances[i], advances[0]);
            adv_consistent = 0;
            pass = 0;
        }
    }
    if (adv_consistent && glyph_count > 1) {
        printf("  PASS ③: All %d advances are consistent (%.2f each) — last glyph not inflated\n",
               glyph_count, advances[0]);
    }

    /* ---- Legend ---- */
    float legend_y = baseline_y + 40;
    cevg_paint_set_style(green, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(green, 3.0f);
    cevg_canvas_draw_line(cv, origin_x, legend_y, origin_x + 30, legend_y, green);

    cevg_paint_set_style(red, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(red, 3.0f);
    cevg_canvas_draw_line(cv, origin_x + 200, legend_y, origin_x + 230, legend_y, red);

    cevg_paint_set_style(blue, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(blue, 3.0f);
    cevg_canvas_draw_line(cv, origin_x + 420, legend_y, origin_x + 450, legend_y, blue);

    free(advances);
    free(pos_x);
    free(pos_y);
    cevg_text_blob_destroy(blob);
    cevg_paint_destroy(white);
    cevg_paint_destroy(red);
    cevg_paint_destroy(green);
    cevg_paint_destroy(blue);
    cevg_paint_destroy(cyan);
    cevg_paint_destroy(orange);
    cevg_paint_destroy(gray);
    cevg_paint_destroy(ltgray);
    return pass;
}

/* =====================================================================
 * Test 2: Per-glyph advance consistency
 * ---------------------------------------------------------------------
 * For various strings, verify that advances are uniform for repeated
 * characters and that the last glyph is not inflated.
 * ===================================================================== */
static int test_advance_consistency(CevgTypeface* face) {
    int pass = 1;
    const char* test_strings[] = {
        "MMMMM",
        "WWWWW",
        "iiiii",
        "Hello World",
        "AV TA",
    };
    int num_tests = sizeof(test_strings) / sizeof(test_strings[0]);

    printf("\n=== Test 2: Advance Consistency ===\n");

    for (int t = 0; t < num_tests; t++) {
        const char* text = test_strings[t];
        size_t len = strlen(text);
        CevgTextBlob* blob = cevg_text_blob_make(text, len, face, 48.0f, kCevgDir_LTR);
        if (!blob) continue;

        int gc = cevg_text_blob_get_glyph_count(blob);
        float* adv = (float*)malloc(gc * sizeof(float));
        cevg_text_blob_get_glyph_advances(blob, adv);

        float width = cevg_text_blob_get_width(blob);
        float sum = 0;
        for (int i = 0; i < gc; i++) sum += adv[i];

        /* Check: last advance should not be wildly different from average
         * (for uniform strings like "MMMMM" it should be identical) */
        float max_adv = 0, min_adv = 1e9;
        for (int i = 0; i < gc; i++) {
            if (adv[i] > max_adv) max_adv = adv[i];
            if (adv[i] < min_adv) min_adv = adv[i];
        }

        int ok = 1;
        /* For uniform strings, max/min ratio should be close to 1 */
        if (t < 3) {  /* "MMMMM", "WWWWW", "iiiii" */
            float ratio = max_adv / (min_adv > 0.01f ? min_adv : 0.01f);
            if (ratio > 1.1f) {
                printf("  FAIL \"%s\": advance ratio max/min = %.3f (should be ~1.0)\n",
                       text, ratio);
                ok = 0;
                pass = 0;
            }
        }

        /* Width should be close to sum of advances (within 10% — kerning
         * can make width < sum_advances, e.g. "AV" has negative kerning) */
        float diff = fabsf(width - sum);
        float tolerance = sum * 0.10f;  /* 10% tolerance for kerning */
        if (tolerance < 3.0f) tolerance = 3.0f;
        if (diff > tolerance) {
            printf("  FAIL \"%s\": width=%.2f vs sum_adv=%.2f (diff=%.2f, tol=%.2f)\n",
                   text, width, sum, diff, tolerance);
            ok = 0;
            pass = 0;
        }

        if (ok) {
            printf("  PASS \"%s\": width=%.2f sum_adv=%.2f adv=[", text, width, sum);
            for (int i = 0; i < gc && i < 6; i++) printf("%.1f ", adv[i]);
            if (gc > 6) printf("...");
            printf("]\n");
        }

        free(adv);
        cevg_text_blob_destroy(blob);
    }
    return pass;
}

/* =====================================================================
 * Test 3: hit_test cursor positioning (visual)
 * ---------------------------------------------------------------------
 * Shape "ABCDE", draw cursor lines at each hit_test boundary,
 * and verify the returned byte offsets.
 * ===================================================================== */
static int test_hit_test_visual(CevgCanvas* cv, CevgTypeface* face,
                                float origin_x, float origin_y) {
    CevgPaint* white  = cevg_paint_create(); set_color(white, COL_WHITE);
    CevgPaint* yellow = cevg_paint_create(); set_color(yellow, COL_YELLOW);
    CevgPaint* orange = cevg_paint_create(); set_color(orange, COL_ORANGE);
    CevgPaint* cyan   = cevg_paint_create(); set_color(cyan, COL_CYAN);
    CevgPaint* green  = cevg_paint_create(); set_color(green, COL_GREEN);
    CevgPaint* red    = cevg_paint_create(); set_color(red, COL_RED);
    CevgPaint* gray   = cevg_paint_create(); set_color(gray, COL_GRAY);

    int pass = 1;
    const char* text = "ABCDE";
    size_t len = strlen(text);
    float fontSize = 64.0f;

    CevgTextBlob* blob = cevg_text_blob_make(text, len, face, fontSize, kCevgDir_LTR);
    if (!blob) { fprintf(stderr, "SKIP: no blob\n"); return 0; }

    float width = cevg_text_blob_get_width(blob);
    int gc = cevg_text_blob_get_glyph_count(blob);

    float* pos_x = (float*)malloc(gc * sizeof(float));
    float* pos_y = (float*)malloc(gc * sizeof(float));
    float* adv   = (float*)malloc(gc * sizeof(float));
    int*   clusters = (int*)malloc(gc * sizeof(int));
    cevg_text_blob_get_glyph_positions(blob, pos_x, pos_y);
    cevg_text_blob_get_glyph_advances(blob, adv);
    cevg_text_blob_get_cluster_info(blob, clusters);

    /* Draw the text */
    float baseline_y = origin_y;
    cevg_canvas_draw_text_blob(cv, blob, origin_x, baseline_y, white);

    /* Draw baseline */
    cevg_paint_set_style(gray, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(gray, 1.0f);
    draw_hline(cv, origin_x - 10, origin_x + width + 30, baseline_y, gray);

    /* Draw cursor positions at each glyph boundary (midpoint between glyphs) */
    cevg_paint_set_style(yellow, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(yellow, 2.0f);

    /* For each glyph, draw its advance box and midpoint */
    for (int i = 0; i < gc; i++) {
        float gx = origin_x + pos_x[i];
        float gr = gx + adv[i];
        float mid = gx + adv[i] * 0.5f;

        /* Draw advance box outline (cyan, thin) */
        cevg_paint_set_style(cyan, kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(cyan, 0.5f);
        CevgFontMetrics fm = {0}; fm.struct_size = sizeof(fm);
        cevg_typeface_get_metrics(face, fontSize, &fm);
        cevg_canvas_draw_rect(cv, gx, baseline_y + fm.ascent,
                              adv[i], fm.descent - fm.ascent, cyan);

        /* Draw midpoint cursor (yellow) */
        cevg_paint_set_style(yellow, kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(yellow, 2.0f);
        draw_vline(cv, mid, baseline_y - 60, baseline_y + 15, yellow, 0);

        /* Draw right-edge cursor (orange, dashed) */
        cevg_paint_set_style(orange, kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(orange, 1.5f);
        draw_vline(cv, gr, baseline_y - 40, baseline_y + 10, orange, 5);
    }

    /* Draw end-of-line cursor (green) */
    cevg_paint_set_style(green, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(green, 3.0f);
    draw_vline(cv, origin_x + width, baseline_y - 70, baseline_y + 20, green, 0);

    /* ---- hit_test validation ---- */
    printf("\n=== Test 3: hit_test (\"ABCDE\" at %.0fpx) ===\n", fontSize);
    printf("  width=%.2f, %d glyphs\n", width, gc);
    for (int i = 0; i < gc; i++) {
        printf("    glyph[%d]: pos_x=%.2f adv=%.2f cluster=%d\n",
               i, pos_x[i], adv[i], clusters[i]);
    }

    /* Test: clicking at each glyph origin should return that glyph's cluster.
     * hit_test uses blob-internal coordinates (positions_x starts at 0). */
    int hit_pass = 1;
    for (int i = 0; i < gc; i++) {
        float test_x = pos_x[i] + adv[i] * 0.25f;  /* left quarter */
        int result = cevg_text_blob_hit_test(blob, test_x, 0);
        if (result != clusters[i]) {
            printf("  FAIL hit_test: x=%.1f (glyph %d left quarter) returned %d, expected %d\n",
                   test_x, i, result, clusters[i]);
            hit_pass = 0;
        }
    }

    /* Test: clicking at right half of each glyph should return next cluster
     * (or text_len for last glyph) */
    for (int i = 0; i < gc; i++) {
        float test_x = pos_x[i] + adv[i] * 0.75f;  /* right quarter */
        int result = cevg_text_blob_hit_test(blob, test_x, 0);
        int expected;
        if (i + 1 < gc)
            expected = clusters[i + 1];
        else
            expected = (int)len;  /* end-of-line */
        if (result != expected) {
            printf("  FAIL hit_test: x=%.1f (glyph %d right quarter) returned %d, expected %d\n",
                   test_x, i, result, expected);
            hit_pass = 0;
        }
    }

    /* Test: clicking past all glyphs should return text_len */
    int eol = cevg_text_blob_hit_test(blob, width + 10, 0);
    if (eol != (int)len) {
        printf("  FAIL hit_test: past-end returned %d, expected %d (text_len)\n", eol, (int)len);
        hit_pass = 0;
    }

    if (hit_pass) {
        printf("  PASS: hit_test returns correct byte offsets at all positions\n");
    } else {
        pass = 0;
    }

    /* ---- Draw hit test markers on canvas ---- */
    /* Red X for failed positions, green dot for passed */
    cevg_paint_set_style(red, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(red, 2.0f);
    cevg_paint_set_style(green, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(green, 2.0f);

    free(pos_x); free(pos_y); free(adv); free(clusters);
    cevg_text_blob_destroy(blob);
    cevg_paint_destroy(white);
    cevg_paint_destroy(yellow);
    cevg_paint_destroy(orange);
    cevg_paint_destroy(cyan);
    cevg_paint_destroy(green);
    cevg_paint_destroy(red);
    cevg_paint_destroy(gray);
    return pass;
}

/* =====================================================================
 * Test 4: Width accuracy — compare against manual calculation
 * ---------------------------------------------------------------------
 * For "MMMMM", the width should be exactly 5 * single_M_advance
 * (no kerning between identical characters).
 * ===================================================================== */
static int test_width_accuracy(CevgTypeface* face) {
    int pass = 1;
    printf("\n=== Test 4: Width Accuracy ===\n");

    /* Single M */
    CevgTextBlob* m1 = cevg_text_blob_make("M", 1, face, 64.0f, kCevgDir_LTR);
    /* Five Ms */
    CevgTextBlob* m5 = cevg_text_blob_make("MMMMM", 5, face, 64.0f, kCevgDir_LTR);

    if (!m1 || !m5) { printf("  SKIP\n"); return 0; }

    float w1 = cevg_text_blob_get_width(m1);
    float w5 = cevg_text_blob_get_width(m5);

    float adv1[1];
    cevg_text_blob_get_glyph_advances(m1, adv1);

    int gc5 = cevg_text_blob_get_glyph_count(m5);
    float* adv5 = (float*)malloc(gc5 * sizeof(float));
    cevg_text_blob_get_glyph_advances(m5, adv5);

    printf("  Single M: width=%.2f advance=%.2f\n", w1, adv1[0]);
    printf("  MMMMM:    width=%.2f advances=[", w5);
    for (int i = 0; i < gc5; i++) printf("%.2f ", adv5[i]);
    printf("]\n");

    /* Width of single M should equal its advance */
    if (fabsf(w1 - adv1[0]) > 1.0f) {
        printf("  FAIL: single M width (%.2f) != advance (%.2f)\n", w1, adv1[0]);
        pass = 0;
    } else {
        printf("  PASS: single M width (%.2f) ≈ advance (%.2f)\n", w1, adv1[0]);
    }

    /* MMMMM width should be 5 * single_M_advance */
    float expected5 = 5 * adv1[0];
    float diff = fabsf(w5 - expected5);
    if (diff > 3.0f) {
        printf("  FAIL: MMMMM width (%.2f) != 5 * M_advance (%.2f), diff=%.2f\n",
               w5, expected5, diff);
        pass = 0;
    } else {
        printf("  PASS: MMMMM width (%.2f) ≈ 5 * M_advance (%.2f), diff=%.2f\n",
               w5, expected5, diff);
    }

    /* All 5 advances should be identical */
    int all_same = 1;
    for (int i = 1; i < gc5; i++) {
        if (fabsf(adv5[i] - adv5[0]) > 0.5f) {
            printf("  FAIL: adv5[%d]=%.2f differs from adv5[0]=%.2f\n", i, adv5[i], adv5[0]);
            all_same = 0;
            pass = 0;
        }
    }
    if (all_same) {
        printf("  PASS: All 5 M advances identical (%.2f)\n", adv5[0]);
    }

    free(adv5);
    cevg_text_blob_destroy(m1);
    cevg_text_blob_destroy(m5);
    return pass;
}

/* =====================================================================
 * Test 5: Visual comparison — draw text with advance-width box
 * ---------------------------------------------------------------------
 * Draw "Hello World" with a green box of width=get_width() behind it,
 * and a red box of width=ink_bounds.width() behind it. The green box
 * should end exactly at the pen position after the last glyph.
 * ===================================================================== */
static int test_visual_boxes(CevgCanvas* cv, CevgTypeface* face,
                             float origin_x, float origin_y) {
    CevgPaint* white  = cevg_paint_create(); set_color(white, COL_WHITE);
    CevgPaint* green  = cevg_paint_create(); set_color(green, 0x6633CC33);  /* semi-transparent green */
    CevgPaint* red    = cevg_paint_create(); set_color(red, 0x66FF3333);    /* semi-transparent red */
    CevgPaint* blue   = cevg_paint_create(); set_color(blue, COL_BLUE);
    CevgPaint* gray   = cevg_paint_create(); set_color(gray, COL_GRAY);

    int pass = 1;
    const char* text = "Hello World";
    size_t len = strlen(text);
    float fontSize = 48.0f;

    CevgTextBlob* blob = cevg_text_blob_make(text, len, face, fontSize, kCevgDir_LTR);
    if (!blob) { fprintf(stderr, "SKIP: no blob\n"); return 0; }

    float width = cevg_text_blob_get_width(blob);
    float ink[4];
    cevg_text_blob_get_ink_bounds(blob, ink);

    CevgFontMetrics fm = {0}; fm.struct_size = sizeof(fm);
    cevg_typeface_get_metrics(face, fontSize, &fm);
    float line_h = fm.descent - fm.ascent;

    float baseline_y = origin_y;

    /* Draw ink bounds box (red, semi-transparent) */
    cevg_paint_set_style(red, kCevgStyle_Fill);
    cevg_canvas_draw_rect(cv, origin_x + ink[0], baseline_y + ink[1],
                          ink[2], ink[3], red);

    /* Draw pen-advance box (green, semi-transparent) */
    cevg_paint_set_style(green, kCevgStyle_Fill);
    cevg_canvas_draw_rect(cv, origin_x, baseline_y + fm.ascent,
                          width, line_h, green);

    /* Draw text on top */
    cevg_canvas_draw_text_blob(cv, blob, origin_x, baseline_y, white);

    /* Draw baseline */
    cevg_paint_set_style(gray, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(gray, 1.0f);
    draw_hline(cv, origin_x - 10, origin_x + ink[0] + ink[2] + 20, baseline_y, gray);

    /* Draw pen-advance right edge (green line) */
    cevg_paint_set_style(blue, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(blue, 2.0f);
    draw_vline(cv, origin_x + width, baseline_y + fm.ascent - 10,
               baseline_y + fm.descent + 10, blue, 0);

    printf("\n=== Test 5: Visual Boxes (\"Hello World\" at %.0fpx) ===\n", fontSize);
    printf("  pen-advance width: %.2f (green box)\n", width);
    printf("  ink bounds: left=%.2f top=%.2f w=%.2f h=%.2f (red box)\n",
           ink[0], ink[1], ink[2], ink[3]);
    printf("  ink right edge: %.2f\n", ink[0] + ink[2]);
    printf("  difference: %.2f (ink is wider by this much)\n",
           (ink[0] + ink[2]) - width);

    cevg_text_blob_destroy(blob);
    cevg_paint_destroy(white);
    cevg_paint_destroy(green);
    cevg_paint_destroy(red);
    cevg_paint_destroy(blue);
    cevg_paint_destroy(gray);
    return pass;
}

/* =====================================================================
 * Test 6: hit_test sweep — test many x positions and print results
 * ===================================================================== */
static int test_hit_test_sweep(CevgTypeface* face) {
    int pass = 1;
    const char* text = "ABCDE";
    size_t len = strlen(text);
    float fontSize = 64.0f;

    CevgTextBlob* blob = cevg_text_blob_make(text, len, face, fontSize, kCevgDir_LTR);
    if (!blob) { printf("  SKIP\n"); return 0; }

    float width = cevg_text_blob_get_width(blob);
    int gc = cevg_text_blob_get_glyph_count(blob);
    float* pos_x = (float*)malloc(gc * sizeof(float));
    float* adv   = (float*)malloc(gc * sizeof(float));
    int*   clusters = (int*)malloc(gc * sizeof(int));
    cevg_text_blob_get_glyph_positions(blob, pos_x, NULL);
    cevg_text_blob_get_glyph_advances(blob, adv);
    cevg_text_blob_get_cluster_info(blob, clusters);

    printf("\n=== Test 6: hit_test Sweep (\"ABCDE\" at %.0fpx) ===\n", fontSize);
    printf("  width=%.2f\n", width);

    /* Sweep from x=-10 to x=width+20 in fine steps */
    int steps = 100;
    float step = (width + 30) / steps;
    int prev_result = -1;
    int transition_count = 0;

    for (int s = 0; s <= steps; s++) {
        float x = -10 + s * step;
        int result = cevg_text_blob_hit_test(blob, x, 0);

        /* Find which glyph this x falls in */
        int in_glyph = -1;
        for (int i = 0; i < gc; i++) {
            if (x >= pos_x[i] && x < pos_x[i] + adv[i]) {
                in_glyph = i;
                break;
            }
        }

        if (result != prev_result) {
            printf("  x=%7.2f  →  byte_offset=%d", x, result);
            if (in_glyph >= 0)
                printf("  (in glyph %d, cluster %d)", in_glyph, clusters[in_glyph]);
            else if (x >= pos_x[gc-1] + adv[gc-1])
                printf("  (past last glyph → EOL)");
            else
                printf("  (before first glyph)");
            printf("\n");
            transition_count++;
            prev_result = result;
        }
    }

    /* Should have transitions at each glyph boundary */
    if (transition_count < gc + 1) {
        printf("  FAIL: expected at least %d transitions, got %d\n", gc + 1, transition_count);
        pass = 0;
    } else {
        printf("  PASS: %d transitions detected across %d glyphs + EOL\n",
               transition_count, gc);
    }

    free(pos_x); free(adv); free(clusters);
    cevg_text_blob_destroy(blob);
    return pass;
}

/* =====================================================================
 * Test 7: Kerning correctness — "AV" has negative kerning
 * ---------------------------------------------------------------------
 * This is the critical test for the max(pos+adv) formula.
 * "AV" has negative kerning: pos_x[V] < pos_x[A] + advance[A].
 * The width should be pos_x[V] + advance[V], NOT pos_x[A] + advance[A].
 *
 * If max() returns the wrong value here, it means the formula is
 * overestimating due to negative kerning.
 * ===================================================================== */
static int test_kerning_width(CevgCanvas* cv, CevgTypeface* face,
                              float origin_x, float origin_y) {
    CevgPaint* white  = cevg_paint_create(); set_color(white, COL_WHITE);
    CevgPaint* green  = cevg_paint_create(); set_color(green, COL_GREEN);
    CevgPaint* red    = cevg_paint_create(); set_color(red, COL_RED);
    CevgPaint* blue   = cevg_paint_create(); set_color(blue, COL_BLUE);
    CevgPaint* yellow = cevg_paint_create(); set_color(yellow, COL_YELLOW);
    CevgPaint* gray   = cevg_paint_create(); set_color(gray, COL_GRAY);
    CevgPaint* magenta = cevg_paint_create(); set_color(magenta, 0xFFFF33FF);

    int pass = 1;
    const char* text = "AV";
    size_t len = strlen(text);
    float fontSize = 64.0f;

    CevgTextBlob* blob = cevg_text_blob_make(text, len, face, fontSize, kCevgDir_LTR);
    if (!blob) { fprintf(stderr, "SKIP: no blob\n"); return 0; }

    float width = cevg_text_blob_get_width(blob);
    int gc = cevg_text_blob_get_glyph_count(blob);

    float* pos_x = (float*)malloc(gc * sizeof(float));
    float* pos_y = (float*)malloc(gc * sizeof(float));
    float* adv   = (float*)malloc(gc * sizeof(float));
    cevg_text_blob_get_glyph_positions(blob, pos_x, pos_y);
    cevg_text_blob_get_glyph_advances(blob, adv);

    float baseline_y = origin_y;

    /* Draw the text */
    cevg_canvas_draw_text_blob(cv, blob, origin_x, baseline_y, white);

    /* Draw baseline */
    cevg_paint_set_style(gray, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(gray, 1.0f);
    draw_hline(cv, origin_x - 10, origin_x + width + 60, baseline_y, gray);

    /* For each glyph, draw its advance cell boundary */
    for (int i = 0; i < gc; i++) {
        float gx = origin_x + pos_x[i];
        float gr = gx + adv[i];

        /* Advance cell outline (blue, thin) */
        CevgFontMetrics fm = {0}; fm.struct_size = sizeof(fm);
        cevg_typeface_get_metrics(face, fontSize, &fm);
        cevg_paint_set_style(blue, kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(blue, 0.5f);
        cevg_canvas_draw_rect(cv, gx, baseline_y + fm.ascent,
                              adv[i], fm.descent - fm.ascent, blue);

        /* Advance right edge (blue dashed) */
        cevg_paint_set_stroke_width(blue, 1.5f);
        draw_vline(cv, gr, baseline_y - 50, baseline_y + 10, blue, 6);
    }

    /* Draw pen-advance width (green) */
    cevg_paint_set_style(green, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(green, 2.0f);
    draw_vline(cv, origin_x + width, baseline_y - 70, baseline_y + 20, green, 0);

    /* Draw where A's advance would end WITHOUT kerning (magenta, dashed)
     * This shows the "nominal" A right edge, which extends past V's origin
     * due to negative kerning */
    float a_nominal_right = origin_x + pos_x[0] + adv[0];
    cevg_paint_set_style(magenta, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(magenta, 1.5f);
    draw_vline(cv, a_nominal_right, baseline_y - 40, baseline_y + 5, magenta, 5);

    /* Draw kerning gap indicator (red arrow) */
    float kerning = pos_x[1] - (pos_x[0] + adv[0]);
    if (kerning < 0) {
        /* Negative kerning: draw a red line showing the overlap */
        float v_left = origin_x + pos_x[1];
        cevg_paint_set_style(red, kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(red, 2.0f);
        draw_hline(cv, v_left, a_nominal_right, baseline_y - 35, red);
    }

    /* ---- Console output ---- */
    printf("\n=== Test 7: Kerning Width (\"AV\" at %.0fpx) ===\n", fontSize);
    printf("  get_width (pen-advance):  %.2f\n", width);
    for (int i = 0; i < gc; i++) {
        printf("    glyph[%d]: pos_x=%.2f  advance=%.2f  pos+adv=%.2f\n",
               i, pos_x[i], adv[i], pos_x[i] + adv[i]);
    }
    printf("  kerning(A→V) = pos_x[V] - (pos_x[A] + adv[A]) = %.2f - %.2f = %.2f\n",
           pos_x[1], pos_x[0] + adv[0], kerning);

    /* The width should equal pos_x[last] + advance[last] for LTR text.
     * With negative kerning, pos_x[A] + adv[A] > pos_x[V], so
     * max() might return pos_x[A] + adv[A] if it's larger than
     * pos_x[V] + adv[V]. Let's check. */
    float a_right = pos_x[0] + adv[0];
    float v_right = pos_x[1] + adv[1];
    float max_val = (a_right > v_right) ? a_right : v_right;

    printf("  A nominal right edge (pos+adv): %.2f\n", a_right);
    printf("  V pen-advance right edge:       %.2f\n", v_right);
    printf("  max(A_right, V_right):          %.2f\n", max_val);
    printf("  get_width():                    %.2f\n", width);

    if (fabsf(width - v_right) < 1.0f) {
        printf("  PASS: width (%.2f) = V's pen-advance edge (%.2f) — correct!\n",
               width, v_right);
        printf("        V is the last glyph; its origin+advance IS the pen-advance width.\n");
    } else if (fabsf(width - max_val) < 1.0f && max_val > v_right) {
        printf("  WARN: width (%.2f) = max(A_right, V_right) = %.2f\n", width, max_val);
        printf("        This overestimates by %.2f because A's nominal right edge\n",
               max_val - v_right);
        printf("        extends past V's origin due to negative kerning.\n");
        printf("        The TRUE pen-advance width should be %.2f (V's edge).\n", v_right);
        printf("        FIX: for LTR text, use pos_x[last]+adv[last] instead of max().\n");
        pass = 0;
    } else {
        printf("  FAIL: width (%.2f) doesn't match either V_right (%.2f) or max (%.2f)\n",
               width, v_right, max_val);
        pass = 0;
    }

    /* Also verify: width should be < sum of advances (kerning makes it shorter) */
    float sum_adv = 0;
    for (int i = 0; i < gc; i++) sum_adv += adv[i];
    if (width > sum_adv) {
        printf("  FAIL: width (%.2f) > sum_advances (%.2f) — kerning should make it shorter!\n",
               width, sum_adv);
        pass = 0;
    } else {
        printf("  PASS: width (%.2f) <= sum_advances (%.2f) — kerning accounted for\n",
               width, sum_adv);
    }

    free(pos_x); free(pos_y); free(adv);
    cevg_text_blob_destroy(blob);
    cevg_paint_destroy(white);
    cevg_paint_destroy(green);
    cevg_paint_destroy(red);
    cevg_paint_destroy(blue);
    cevg_paint_destroy(yellow);
    cevg_paint_destroy(gray);
    cevg_paint_destroy(magenta);
    return pass;
}

/* =====================================================================
 * Main
 * ===================================================================== */
int main() {
    printf("=== Cevg Text Measurement Verification ===\n");

    CevgContext* ctx = cevg_test_init(1200, 1100);
    if (!ctx) return 1;

    CevgCanvas* cv = cevg_test_canvas();

    /* Clear to dark background */
    cevg_canvas_clear(cv, 0.1f, 0.1f, 0.12f, 1.0f);

    /* Load font */
    CevgTypeface* face = cevg_typeface_create_from_file(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);
    if (!face) {
        fprintf(stderr, "SKIP: DejaVuSans.ttf not found\n");
        cevg_test_save_png("test_text_measure.png");
        cevg_test_shutdown();
        return 0;
    }

    int total_pass = 1;

    /* Test 1: Width vs Ink Bounds (visual) — top section */
    total_pass &= test_width_vs_ink(cv, face, 80, 120);

    /* Test 3: hit_test visual — middle section */
    total_pass &= test_hit_test_visual(cv, face, 80, 340);

    /* Test 7: Kerning width — "AV" with negative kerning */
    total_pass &= test_kerning_width(cv, face, 80, 560);

    /* Test 5: Visual boxes — bottom section */
    total_pass &= test_visual_boxes(cv, face, 80, 780);

    /* Test 2: Advance consistency (console only) */
    total_pass &= test_advance_consistency(face);

    /* Test 4: Width accuracy (console only) */
    total_pass &= test_width_accuracy(face);

    /* Test 6: hit_test sweep (console only) */
    total_pass &= test_hit_test_sweep(face);

    /* Save PNG */
    cevg_test_save_png("test_text_measure.png");

    cevg_typeface_unref(face);
    cevg_test_shutdown();

    printf("\n=== Overall: %s ===\n", total_pass ? "ALL PASS" : "SOME FAILED");
    return total_pass ? 0 : 1;
}
