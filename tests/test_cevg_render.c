/* =====================================================================
 * test_cevg_render.c — Cevg rendering test suite (Skia Graphite backend)
 * ---------------------------------------------------------------------
 * Generates a 4K (3840x2160) PNG with a 16x9 grid of test cells.
 * Each cell is 240x240px with 4px internal padding.
 * Cell labels (P01, P02, ...) are drawn in the top-left corner.
 *
 * This file implements P01-P144: Basic Shapes, Paths, Stroke caps/joins,
 * Dashes, Transforms, Clipping, Blend Modes, Gradients, Layers/Filters,
 * Image Drawing, Image Shader, Text Rendering, Display List, Comprehensive Scenes
 * ===================================================================== */
#define _USE_MATH_DEFINES

#include "cevg_test_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- Grid layout constants ---- */
#define FB_W       3840
#define FB_H       2160
#define COLS       16
#define ROWS       9
#define CELL_W     240
#define CELL_H     240
#define PAD        4

/* Background color: #1A1A2E */
#define BG_R  0.102f
#define BG_G  0.102f
#define BG_B  0.180f
#define BG_A  1.0f

/* ---- Helper: get cell origin (top-left of content area, after padding) ---- */
static void cell_origin(int col, int row, float* ox, float* oy) {
    *ox = (float)(col * CELL_W + PAD);
    *oy = (float)(row * CELL_H + PAD);
}

/* Content area size (cell minus padding on both sides) */
#define CONTENT_W  (CELL_W - PAD * 2)
#define CONTENT_H  (CELL_H - PAD * 2)

/* ---- Helper: draw cell label (e.g. "P01") ---- */
static void draw_label(CevgCanvas* cv, CevgTypeface* face, int col, int row,
                       const char* text) {
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    CevgTextBlob* blob = cevg_text_blob_make(text, strlen(text), face, 11.0f,
                                             kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.7f, 0.7f, 0.8f, 0.9f);
    cevg_canvas_draw_text_blob(cv, blob, ox + 2, oy + 2, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

/* ---- Helper: draw cell border ---- */
static void draw_cell_border(CevgCanvas* cv, int col, int row) {
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 1.0f);
    cevg_paint_set_color(p, 0.25f, 0.25f, 0.35f, 0.6f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_paint_destroy(p);
}

/* ================================================================== */
/* P01-P12: Basic Shapes                                              */
/* ================================================================== */

static void p01_fill_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P01");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);  /* #FF4444 */
    /* 160x100 centered in cell */
    float cx = ox + (CONTENT_W - 160) * 0.5f;
    float cy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_canvas_draw_rect(cv, cx, cy, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p02_fill_rect_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P02");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 0.5f);  /* #FF4444 alpha=0.5 */
    float cx = ox + (CONTENT_W - 160) * 0.5f;
    float cy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_canvas_draw_rect(cv, cx, cy, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p03_stroke_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P03");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);  /* #44FF44 */
    float cx = ox + (CONTENT_W - 160) * 0.5f;
    float cy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_canvas_draw_rect(cv, cx, cy, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p04_fill_round_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P04");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);  /* #4488FF */
    float cx = ox + (CONTENT_W - 160) * 0.5f;
    float cy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_canvas_draw_round_rect(cv, cx, cy, 160, 100, 20, 20, p);
    cevg_paint_destroy(p);
}

static void p05_stroke_round_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P05");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 1.0f, 1.0f, 0.267f, 1.0f);  /* #FFFF44 */
    float cx = ox + (CONTENT_W - 160) * 0.5f;
    float cy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_canvas_draw_round_rect(cv, cx, cy, 160, 100, 20, 20, p);
    cevg_paint_destroy(p);
}

static void p06_fill_circle(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P06");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 1.0f, 1.0f);  /* #FF44FF */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    cevg_canvas_draw_circle(cv, cx, cy, 60, p);
    cevg_paint_destroy(p);
}

static void p07_stroke_circle(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P07");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 0.267f, 1.0f, 1.0f, 1.0f);  /* #44FFFF */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    cevg_canvas_draw_circle(cv, cx, cy, 60, p);
    cevg_paint_destroy(p);
}

static void p08_fill_oval(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P08");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.267f, 1.0f);  /* #FF8844 */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    cevg_canvas_draw_oval(cv, cx, cy, 80, 40, p);
    cevg_paint_destroy(p);
}

static void p09_stroke_oval(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P09");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 0.533f, 0.267f, 1.0f, 1.0f);  /* #8844FF */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    cevg_canvas_draw_oval(cv, cx, cy, 80, 40, p);
    cevg_paint_destroy(p);
}

static void p10_draw_line(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P10");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 2.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Butt);
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);  /* #FFFFFF */
    cevg_canvas_draw_line(cv, ox + 30, oy + 30, ox + 210, oy + 210, p);
    cevg_paint_destroy(p);
}

static void p11_draw_line_thick(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P11");

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 8.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Butt);
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);  /* #FF4444 */
    cevg_canvas_draw_line(cv, ox + 30, oy + 210, ox + 210, oy + 30, p);
    cevg_paint_destroy(p);
}

static void p12_draw_paint(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P12");

    /* Clip to cell content area, then fill with paint */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.2f, 0.4f, 0.6f, 1.0f);  /* #336699 */
    cevg_canvas_draw_paint(cv, p);
    cevg_paint_destroy(p);

    cevg_canvas_restore(cv);
}

/* ================================================================== */
/* P13-P24: Paths                                                      */
/* ================================================================== */

static void p13_path_triangle(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P13");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 120, oy + 20);
    cevg_path_line_to(path, ox + 20,  oy + 220);
    cevg_path_line_to(path, ox + 220, oy + 220);
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 0.8f, 0.267f, 1.0f);  /* #44CC44 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p14_path_pentagon(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P14");
    (void)face;

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float r = 80.0f;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 5; i++) {
        float angle = (float)(-M_PI / 2.0) + i * 2.0f * (float)M_PI / 5.0f;
        float px = cx + r * cosf(angle);
        float py = cy + r * sinf(angle);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.8f, 0.267f, 0.8f, 1.0f);  /* #CC44CC */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p15_path_star(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P15");
    (void)face;

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float outer = 90.0f;
    float inner = 36.0f;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float angle = (float)(-M_PI / 2.0) + i * (float)M_PI / 5.0f;
        float r = (i % 2 == 0) ? outer : inner;
        float px = cx + r * cosf(angle);
        float py = cy + r * sinf(angle);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.667f, 0.0f, 1.0f);  /* #FFAA00 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p16_path_star_eodd(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 0;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P16");
    (void)face;

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float outer = 90.0f;
    float inner = 36.0f;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float angle = (float)(-M_PI / 2.0) + i * (float)M_PI / 5.0f;
        float r = (i % 2 == 0) ? outer : inner;
        float px = cx + r * cosf(angle);
        float py = cy + r * sinf(angle);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);
    cevg_path_set_fill_rule(path, kCevgFillRule_EvenOdd);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.667f, 0.0f, 1.0f);  /* #FFAA00 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p17_path_quad_bezier(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P17");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 20, oy + 200);
    cevg_path_quad_to(path, ox + 120, oy + 20, ox + 220, oy + 200);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 0.267f, 0.667f, 1.0f, 1.0f);  /* #44AAFF */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p18_path_cubic_bezier(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P18");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 20, oy + 180);
    cevg_path_cubic_to(path, ox + 60, oy + 20, ox + 180, oy + 20, ox + 220, oy + 180);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.667f, 1.0f);  /* #FF44AA */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p19_path_stroke(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P19");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 20, oy + 120);
    cevg_path_line_to(path, ox + 80, oy + 40);
    cevg_path_line_to(path, ox + 140, oy + 120);
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 4.0f);
    cevg_paint_set_color(p, 0.667f, 1.0f, 0.267f, 1.0f);  /* #AAFF44 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p20_path_multi_contour(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P20");
    (void)face;

    float lcx = ox + 70, lcy = oy + 120, lr = 40.0f;
    float rcx = ox + 170, rcy = oy + 120, rr = 40.0f;
    int segs = 32;

    CevgPath* path = cevg_path_create();
    /* Left circle */
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = lcx + lr * cosf(a);
        float py = lcy + lr * sinf(a);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);
    /* Right circle */
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = rcx + rr * cosf(a);
        float py = rcy + rr * sinf(a);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.667f, 1.0f);  /* #44FFAA */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p21_path_concave(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P21");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 120, oy + 20);
    cevg_path_line_to(path, ox + 200, oy + 100);
    cevg_path_line_to(path, ox + 140, oy + 100);
    cevg_path_line_to(path, ox + 140, oy + 220);
    cevg_path_line_to(path, ox + 100, oy + 220);
    cevg_path_line_to(path, ox + 100, oy + 100);
    cevg_path_line_to(path, ox + 40,  oy + 100);
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.4f, 0.267f, 1.0f);  /* #FF6644 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p22_path_complex(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P22");
    (void)face;

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    int turns = 3;
    int steps = turns * 40;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float angle = t * turns * 2.0f * (float)M_PI;
        float r = 5.0f + t * 100.0f;
        float px = cx + r * cosf(angle);
        float py = cy + r * sinf(angle);
        if (i == 0)
            cevg_path_move_to(path, px, py);
        else
            cevg_path_line_to(path, px, py);
    }

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 2.0f);
    cevg_paint_set_color(p, 0.667f, 0.867f, 1.0f, 1.0f);  /* #AADDFF */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p23_path_dash(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P23");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, rx, ry);
    cevg_path_line_to(path, rx + 160, ry);
    cevg_path_line_to(path, rx + 160, ry + 100);
    cevg_path_line_to(path, rx, ry + 100);
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    float dashes[] = { 10.0f, 5.0f };
    cevg_paint_set_dash(p, dashes, 2, 0.0f);
    cevg_paint_set_color(p, 1.0f, 0.533f, 1.0f, 1.0f);  /* #FF88FF */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p24_path_rewind(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P24");
    (void)face;

    CevgPath* path = cevg_path_create();
    /* Triangle */
    cevg_path_move_to(path, ox + 120, oy + 20);
    cevg_path_line_to(path, ox + 40,  oy + 200);
    cevg_path_line_to(path, ox + 200, oy + 200);
    cevg_path_close(path);

    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 0.267f, 0.533f, 0.267f, 1.0f);  /* #448844 */
    cevg_canvas_draw_path(cv, path, p1);
    cevg_paint_destroy(p1);

    /* Rewind and draw square */
    cevg_path_rewind(path);
    float sx = ox + (CONTENT_W - 100) * 0.5f;
    float sy = oy + (CONTENT_H - 100) * 0.5f;
    cevg_path_move_to(path, sx, sy);
    cevg_path_line_to(path, sx + 100, sy);
    cevg_path_line_to(path, sx + 100, sy + 100);
    cevg_path_line_to(path, sx, sy + 100);
    cevg_path_close(path);

    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.533f, 0.267f, 0.267f, 1.0f);  /* #884444 */
    cevg_canvas_draw_path(cv, path, p2);
    cevg_paint_destroy(p2);
    cevg_path_destroy(path);
}

/* ================================================================== */
/* P25-P32: Stroke caps/joins                                          */
/* ================================================================== */

static void p25_cap_butt(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P25");
    (void)face;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 12.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Butt);
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);  /* #FF4444 */

    float ys[] = { 60.0f, 120.0f, 180.0f };
    for (int i = 0; i < 3; i++)
        cevg_canvas_draw_line(cv, ox + 30, oy + ys[i], ox + 210, oy + ys[i], p);

    cevg_paint_destroy(p);
}

static void p26_cap_round(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P26");
    (void)face;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 12.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Round);
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);  /* #44FF44 */

    float ys[] = { 60.0f, 120.0f, 180.0f };
    for (int i = 0; i < 3; i++)
        cevg_canvas_draw_line(cv, ox + 30, oy + ys[i], ox + 210, oy + ys[i], p);

    cevg_paint_destroy(p);
}

static void p27_cap_square(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P27");
    (void)face;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 12.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Square);
    cevg_paint_set_color(p, 0.267f, 0.267f, 1.0f, 1.0f);  /* #4444FF */

    float ys[] = { 60.0f, 120.0f, 180.0f };
    for (int i = 0; i < 3; i++)
        cevg_canvas_draw_line(cv, ox + 30, oy + ys[i], ox + 210, oy + ys[i], p);

    cevg_paint_destroy(p);
}

static void p28_join_miter(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P28");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 30,  oy + 180);
    cevg_path_line_to(path, ox + 120, oy + 40);
    cevg_path_line_to(path, ox + 210, oy + 180);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 10.0f);
    cevg_paint_set_stroke_join(p, kCevgJoin_Miter);
    cevg_paint_set_color(p, 1.0f, 1.0f, 0.267f, 1.0f);  /* #FFFF44 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p29_join_round(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P29");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 30,  oy + 180);
    cevg_path_line_to(path, ox + 120, oy + 40);
    cevg_path_line_to(path, ox + 210, oy + 180);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 10.0f);
    cevg_paint_set_stroke_join(p, kCevgJoin_Round);
    cevg_paint_set_color(p, 0.267f, 1.0f, 1.0f, 1.0f);  /* #44FFFF */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p30_join_bevel(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P30");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 30,  oy + 180);
    cevg_path_line_to(path, ox + 120, oy + 40);
    cevg_path_line_to(path, ox + 210, oy + 180);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 10.0f);
    cevg_paint_set_stroke_join(p, kCevgJoin_Bevel);
    cevg_paint_set_color(p, 1.0f, 0.267f, 1.0f, 1.0f);  /* #FF44FF */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p31_miter_limit(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P31");
    (void)face;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, ox + 30,  oy + 100);
    cevg_path_line_to(path, ox + 120, oy + 95);
    cevg_path_line_to(path, ox + 210, oy + 100);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 8.0f);
    cevg_paint_set_stroke_join(p, kCevgJoin_Miter);
    cevg_paint_set_stroke_miter(p, 2.0f);
    cevg_paint_set_color(p, 1.0f, 0.667f, 0.267f, 1.0f);  /* #FFAA44 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p32_stroke_hairline(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 1;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P32");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 0.5f);
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);  /* #FFFFFF */
    cevg_canvas_draw_rect(cv, rx, ry, 160, 100, p);
    cevg_paint_destroy(p);
}

/* ================================================================== */
/* P33-P36: Dashes                                                     */
/* ================================================================== */

static void p33_dash_even(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P33");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    float dashes[] = { 5.0f, 5.0f };
    cevg_paint_set_dash(p, dashes, 2, 0.0f);
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.533f, 1.0f);  /* #44FF88 */
    cevg_canvas_draw_rect(cv, rx, ry, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p34_dash_uneven(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P34");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    float dashes[] = { 10.0f, 4.0f, 2.0f, 4.0f };
    cevg_paint_set_dash(p, dashes, 4, 0.0f);
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.267f, 1.0f);  /* #FF8844 */
    cevg_canvas_draw_rect(cv, rx, ry, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p35_dash_phase(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P35");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    float dashes[] = { 5.0f, 5.0f };
    cevg_paint_set_dash(p, dashes, 2, 5.0f);
    cevg_paint_set_color(p, 0.533f, 0.267f, 1.0f, 1.0f);  /* #8844FF */
    cevg_canvas_draw_rect(cv, rx, ry, 160, 100, p);
    cevg_paint_destroy(p);
}

static void p36_dash_round_cap(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P36");
    (void)face;

    float rx = ox + (CONTENT_W - 160) * 0.5f;
    float ry = oy + (CONTENT_H - 100) * 0.5f;

    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, rx, ry);
    cevg_path_line_to(path, rx + 160, ry);
    cevg_path_line_to(path, rx + 160, ry + 100);
    cevg_path_line_to(path, rx, ry + 100);
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 6.0f);
    cevg_paint_set_stroke_cap(p, kCevgCap_Round);
    float dashes[] = { 8.0f, 8.0f };
    cevg_paint_set_dash(p, dashes, 2, 0.0f);
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.533f, 1.0f);  /* #FF4488 */
    cevg_canvas_draw_path(cv, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

/* ================================================================== */
/* P37-P48: Transforms                                                 */
/* ================================================================== */

static void p37_translate(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P37");

    /* Red rect at local origin */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 1.0f);  /* #FF4444 */
    cevg_canvas_draw_rect(cv, ox, oy, 80, 60, p1);

    /* Green rect after translate */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 80, oy + 90);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 1.0f);  /* #44FF44 */
    cevg_canvas_draw_rect(cv, 0, 0, 80, 60, p2);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p38_scale_uniform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P38");

    /* Small red rect */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, 40, 30, p1);

    /* 3x scaled green rect */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox, oy + 35);
    cevg_canvas_scale(cv, 3.0f, 3.0f);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, 0, 0, 40, 30, p2);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p39_scale_nonuniform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P39");

    /* Small red square */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, 40, 40, p1);

    /* 3x horizontal, 1x vertical green */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox, oy + 45);
    cevg_canvas_scale(cv, 3.0f, 1.0f);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, 0, 0, 40, 40, p2);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p40_rotate_45(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P40");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;

    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, cx, cy);
    cevg_canvas_rotate(cv, 45.0f);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 0.667f, 1.0f, 1.0f);  /* #44AAFF */
    cevg_canvas_draw_rect(cv, -30, -30, 60, 60, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p41_rotate_90(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P41");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;

    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, cx, cy);
    cevg_canvas_rotate(cv, 90.0f);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.667f, 0.267f, 1.0f);  /* #FFAA44 */
    cevg_canvas_draw_rect(cv, -60, -15, 120, 30, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p42_skew(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P42");

    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 60, oy + 60);
    cevg_canvas_skew(cv, 0.3f, 0.0f);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.667f, 0.267f, 1.0f, 1.0f);  /* #AA44FF */
    cevg_canvas_draw_rect(cv, 0, 0, 100, 80, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p43_concat_matrix(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P43");

    /* Y-shear matrix: [1,0,0, 0.5,1,0, 0,0,1] column-major */
    float mat[9] = { 1.0f, 0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f };

    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 40, oy + 40);
    cevg_canvas_concat(cv, mat);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.667f, 1.0f);  /* #44FFAA */
    cevg_canvas_draw_rect(cv, 0, 0, 100, 80, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p44_reset_matrix(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P44");

    /* Rotated red rect */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 60, oy + 60);
    cevg_canvas_rotate(cv, 30.0f);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 0.7f);
    cevg_canvas_draw_rect(cv, 0, 0, 80, 50, p1);
    cevg_canvas_restore(cv);

    /* Reset matrix green rect (no rotation) */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 40, oy + 130);
    cevg_canvas_reset_matrix(cv);
    /* After reset_matrix we need to re-translate since CTM is identity */
    cevg_canvas_translate(cv, ox + 40, oy + 130);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 0.7f);
    cevg_canvas_draw_rect(cv, 0, 0, 80, 50, p2);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p45_save_restore(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P45");

    /* Red rect after translate */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 50, oy + 50);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, 0, 0, 80, 50, p1);
    cevg_canvas_restore(cv);

    /* Green rect without translate (at cell origin) */
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 10, oy + 130, 80, 50, p2);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p46_nested_transform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P46");

    /* Red rect with translate + rotate + scale */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 80, oy + 80);
    cevg_canvas_rotate(cv, 30.0f);
    cevg_canvas_scale(cv, 1.5f, 1.5f);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 0.8f);
    cevg_canvas_draw_rect(cv, -30, -20, 60, 40, p1);
    cevg_canvas_restore(cv);

    /* Green rect without transforms */
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 0.8f);
    cevg_canvas_draw_rect(cv, ox + 10, oy + 160, 60, 40, p2);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p47_transform_line(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P47");

    /* Line rotated 45 degrees */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + CONTENT_W * 0.5f, oy + CONTENT_H * 0.5f);
    cevg_canvas_rotate(cv, 45.0f);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 3.0f);
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.267f, 1.0f);  /* #FF8844 */
    cevg_canvas_draw_line(cv, -60, 0, 60, 0, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p48_transform_path(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 2;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P48");

    /* Small triangle, then 2x scaled */
    CevgPath* path = cevg_path_create();
    cevg_path_move_to(path, 30, 0);
    cevg_path_line_to(path, 0, 50);
    cevg_path_line_to(path, 60, 50);
    cevg_path_close(path);

    /* Red: original size */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 10, oy + 10);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 0.6f);
    cevg_canvas_draw_path(cv, path, p1);
    cevg_canvas_restore(cv);

    /* Green: 2x scaled */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 50, oy + 50);
    cevg_canvas_scale(cv, 2.0f, 2.0f);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 0.6f);
    cevg_canvas_draw_path(cv, path, p2);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
    cevg_path_destroy(path);
}

/* ================================================================== */
/* P49-P56: Clipping                                                   */
/* ================================================================== */

static void p49_clip_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P49");

    /* Big blue rect, clipped to center 100x100 */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox + 66, oy + 66, 100, 100);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);  /* #4488FF */
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p50_clip_rect_nested(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P50");

    /* Two nested clip rects, fill red — only intersection visible */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox + 20, oy + 20, 200, 200);
    cevg_canvas_clip_rect(cv, ox + 60, oy + 60, 120, 120);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p51_clip_path(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P51");

    /* Circle clip, fill green — only inside circle visible */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float r = 70.0f;
    int segs = 48;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = cx + r * cosf(a);
        float py = cy + r * sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    cevg_canvas_save(cv);
    cevg_canvas_clip_path(cv, path);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);  /* #44FF44 */
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p52_clip_path_star(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P52");

    /* Star clip (even-odd), fill yellow — star visible, center hollow */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float outer = 90.0f, inner = 36.0f;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float a = (float)(-M_PI / 2.0) + i * (float)M_PI / 5.0f;
        float rr = (i % 2 == 0) ? outer : inner;
        float px = cx + rr * cosf(a);
        float py = cy + rr * sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);
    cevg_path_set_fill_rule(path, kCevgFillRule_EvenOdd);

    cevg_canvas_save(cv);
    cevg_canvas_clip_path(cv, path);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 0.267f, 1.0f);  /* #FFFF44 */
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p53_clip_transform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P53");

    /* Clip rect after translate — clip region is offset */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, 60, 60);
    cevg_canvas_clip_rect(cv, ox, oy, 80, 80);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p54_clip_save_restore(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P54");

    /* Clipped red, then restore and unclipped green */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox + 20, oy + 20, 80, 80);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p1);
    cevg_canvas_restore(cv);

    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.267f, 1.0f, 0.267f, 0.5f);
    cevg_canvas_draw_rect(cv, ox + 80, oy + 80, 100, 100, p2);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p55_clip_multiple(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P55");

    /* Draw a green border rect as reference (outside clip) */
    CevgPaint* p0 = cevg_paint_create();
    cevg_paint_set_style(p0, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p0, 2.0f);
    cevg_paint_set_color(p0, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 180, p0);

    /* Left half clip + right half clip = nearly empty intersection */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, 116, (float)CONTENT_H);     /* left half */
    cevg_canvas_clip_rect(cv, ox + 116, oy, 116, (float)CONTENT_H); /* right half */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p0);
    cevg_paint_destroy(p);
}

static void p56_clip_empty(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 3;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);

    draw_label(cv, face, col, row, "P56");

    /* Draw a green border rect as reference (outside clip) */
    CevgPaint* p0 = cevg_paint_create();
    cevg_paint_set_style(p0, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p0, 2.0f);
    cevg_paint_set_color(p0, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 180, p0);

    /* Zero-size clip — nothing visible inside */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, 0, 0);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p0);
    cevg_paint_destroy(p);
}

/* ================================================================== */
/* P57-P72: Blend Modes                                                */
/* ================================================================== */

/* Helper: draw two overlapping rects with a specific blend mode.
 * Red rect on left half, blue rect on right half (overlapping red by half). */
static void draw_blend_test(CevgCanvas* cv, CevgTypeface* face,
                            int col, int row, const char* label,
                            CevgBlendMode mode) {
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, label);

    float rw = 100, rh = 140;
    float ry = oy + 40;

    /* Red rect: left half */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.0f, 0.0f, 1.0f);  /* pure red */
    cevg_canvas_draw_rect(cv, ox + 16, ry, rw, rh, p1);

    /* Blue rect: right half, overlapping red by half, with blend mode */
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.0f, 0.0f, 1.0f, 1.0f);  /* pure blue */
    cevg_paint_set_blend_mode(p2, mode);
    cevg_canvas_draw_rect(cv, ox + 16 + rw * 0.5f, ry, rw, rh, p2);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p57_blend_srcover(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 8, 3, "P57", kCevgBlendMode_SrcOver);
}
static void p58_blend_src(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 9, 3, "P58", kCevgBlendMode_Src);
}
static void p59_blend_dst(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 10, 3, "P59", kCevgBlendMode_Dst);
}
static void p60_blend_clear(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 11, 3, "P60", kCevgBlendMode_Clear);
}
static void p61_blend_src_in(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 12, 3, "P61", kCevgBlendMode_SrcIn);
}
static void p62_blend_dst_in(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 13, 3, "P62", kCevgBlendMode_DstIn);
}
static void p63_blend_src_out(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 14, 3, "P63", kCevgBlendMode_SrcOut);
}
static void p64_blend_dst_out(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 15, 3, "P64", kCevgBlendMode_DstOut);
}
static void p65_blend_src_atop(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 0, 4, "P65", kCevgBlendMode_SrcATop);
}
static void p66_blend_xor(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 1, 4, "P66", kCevgBlendMode_Xor);
}
static void p67_blend_screen(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 2, 4, "P67", kCevgBlendMode_Screen);
}
static void p68_blend_multiply(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 3, 4, "P68", kCevgBlendMode_Multiply);
}
static void p69_blend_overlay(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 4, 4, "P69", kCevgBlendMode_Overlay);
}
static void p70_blend_darken(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 5, 4, "P70", kCevgBlendMode_Darken);
}
static void p71_blend_lighten(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 6, 4, "P71", kCevgBlendMode_Lighten);
}
static void p72_blend_difference(CevgCanvas* cv, CevgTypeface* face) {
    draw_blend_test(cv, face, 7, 4, "P72", kCevgBlendMode_Difference);
}

/* ================================================================== */
/* P73-P88: Gradients                                                  */
/* ================================================================== */

static void p73_grad_linear_2stop(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P73");

    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };  /* red -> blue */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 216, oy + 40,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p74_grad_linear_3stop(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P74");

    uint32_t colors[] = { 0xFFFF0000, 0xFFFFFF00, 0xFF0000FF };  /* red -> yellow -> blue */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 216, oy + 40,
                                   colors, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p75_grad_linear_vertical(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P75");

    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };  /* white -> black */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 60, oy + 20, ox + 60, oy + 210,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 20, 100, 190, p);
    cevg_paint_destroy(p);
}

static void p76_grad_linear_diagonal(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P76");

    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };  /* red -> blue */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 16, ox + 216, oy + 216,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p77_grad_linear_clamp(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P77");

    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };  /* red -> blue */
    CevgPaint* p = cevg_paint_create();
    /* Gradient covers only left half, clamp extends right edge color */
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 116, oy + 40,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p78_grad_linear_repeat(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P78");

    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 116, oy + 40,
                                   colors, NULL, 2, kCevgTile_Repeat);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p79_grad_linear_mirror(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P79");

    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 116, oy + 40,
                                   colors, NULL, 2, kCevgTile_Mirror);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p80_grad_linear_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 4;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P80");

    uint32_t colors[] = { 0xFFFF0000FF, 0xFF0000FF00 };  /* opaque red -> transparent blue */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 16, oy + 40, ox + 216, oy + 40,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 40, 200, 140, p);
    cevg_paint_destroy(p);
}

static void p81_grad_radial_2stop(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P81");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };  /* red center -> blue edge */

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 90.0f, colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p82_grad_radial_3stop(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P82");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFF0000, 0xFFFFFF00, 0xFF0000FF };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 90.0f, colors, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p83_grad_radial_center(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P83");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };  /* white center -> black edge */

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 100.0f, colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p84_grad_radial_offset(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P84");

    /* Off-center: center at upper-left area */
    float cx = ox + 60;
    float cy = oy + 60;
    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 100.0f, colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p85_grad_radial_clamp(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P85");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };  /* small radius */

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 40.0f, colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p86_grad_radial_repeat(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P86");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 40.0f, colors, NULL, 2, kCevgTile_Repeat);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p87_grad_radial_mirror(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P87");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFFFFFF, 0xFF000000 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 40.0f, colors, NULL, 2, kCevgTile_Mirror);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

static void p88_grad_radial_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P88");

    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    uint32_t colors[] = { 0xFFFF0000FF, 0xFF0000FF00 };  /* opaque red -> transparent blue */

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 90.0f, colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 16, 200, 200, p);
    cevg_paint_destroy(p);
}

/* ================================================================== */
/* P89-P108: Layers, Filters, Backdrop Filters                        */
/* ================================================================== */

static void p89_layer_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P89");

    /* Red rect behind layer */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 80, 140, p1);

    /* Layer with 0.5 alpha, draw green rect inside */
    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    cevg_canvas_save_layer(cv, 0.5f, bounds, NULL);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.0f, 1.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 60, 80, 140, p2);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p90_layer_blend(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P90");

    /* Red rect behind */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 100, 140, p1);

    /* Layer with Screen blend mode */
    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blend_mode(lp, kCevgBlendMode_Screen);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 70, oy + 60, 100, 140, p2);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
    cevg_paint_destroy(lp);
}

static void p91_layer_nested(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P91");

    /* Outer layer alpha 0.7 */
    float b1[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    cevg_canvas_save_layer(cv, 0.7f, b1, NULL);

    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 80, 140, p1);

    /* Inner layer alpha 0.5 */
    float b2[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    cevg_canvas_save_layer(cv, 0.5f, b2, NULL);

    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.0f, 1.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 60, 80, 140, p2);

    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p1);
    cevg_paint_destroy(p2);
}

static void p92_layer_empty(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P92");

    /* Red rect behind */
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 30, oy + 50, 160, 130, p1);

    /* Empty layer — nothing drawn inside, should not affect background */
    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    cevg_canvas_save_layer(cv, 1.0f, bounds, NULL);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p1);
}

static void p93_filter_blur(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P93");

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blur(lp, 8.0f, 8.0f);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 40, oy + 50, 60, 60, p);
    cevg_paint_set_color(p, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 120, oy + 100, 60, 60, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p94_filter_blur_large(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P94");

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blur(lp, 20.0f, 20.0f);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 60, 100, 100, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p95_filter_drop_shadow(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P95");

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_drop_shadow(lp, 4.0f, 4.0f, 3.0f, 0xFF000000);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.8f, 0.0f, 1.0f);  /* yellow */
    cevg_canvas_draw_rect(cv, ox + 50, oy + 50, 100, 100, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p96_filter_drop_shadow_color(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 5;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P96");

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_drop_shadow(lp, 3.0f, 3.0f, 4.0f, 0xFFFF0000);  /* red shadow */
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);  /* white rect */
    cevg_canvas_draw_rect(cv, ox + 50, oy + 50, 100, 100, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p97_filter_color_matrix(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P97");

    /* Grayscale color matrix */
    float mat[20] = {
        0.299f, 0.587f, 0.114f, 0.0f, 0.0f,
        0.299f, 0.587f, 0.114f, 0.0f, 0.0f,
        0.299f, 0.587f, 0.114f, 0.0f, 0.0f,
        0.0f,   0.0f,   0.0f,   1.0f, 0.0f
    };

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    /* Draw colorful rects */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 60, 60, p);
    cevg_paint_set_color(p, 0.0f, 1.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 90, oy + 40, 60, 60, p);
    cevg_paint_set_color(p, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 55, oy + 110, 60, 60, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p98_filter_color_matrix_invert(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P98");

    /* Invert color matrix */
    float mat[20] = {
        -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,-1.0f,  0.0f,  0.0f, 1.0f,
         0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
         0.0f, 0.0f,  0.0f,  1.0f, 0.0f
    };

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 180, 150, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p99_filter_blur_and_shadow(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P99");

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blur(lp, 4.0f, 4.0f);
    cevg_paint_set_drop_shadow(lp, 3.0f, 3.0f, 3.0f, 0xFF000000);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.0f, 1.0f, 0.533f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 50, oy + 50, 100, 100, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p100_backdrop_blur(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P100");

    /* Draw colorful background */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 20, 90, 90, p);
    cevg_paint_set_color(p, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 120, oy + 20, 90, 90, p);
    cevg_paint_set_color(p, 0.0f, 1.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 100, 90, 90, p);

    /* Backdrop blur layer */
    float bounds[4] = { ox + 40, oy + 40, ox + 190, oy + 170 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, 10.0f, 10.0f);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p101_backdrop_blur_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P101");

    /* Colorful background */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 20, 200, 180, p);

    /* Backdrop blur with 0.5 alpha layer */
    float bounds[4] = { ox + 40, oy + 40, ox + 190, oy + 170 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, 8.0f, 8.0f);
    cevg_canvas_save_layer(cv, 0.5f, bounds, lp);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p102_backdrop_shadow(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P102");

    /* White background */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 20, 200, 180, p);

    /* Backdrop shadow layer */
    float bounds[4] = { ox + 40, oy + 40, ox + 190, oy + 170 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_backdrop_shadow(lp, 4.0f, 4.0f, 6.0f, 0xFF000000);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p103_layer_clip(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P103");

    /* Clip to center region, then layer with blur */
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox + 40, oy + 40, 150, 150);

    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blur(lp, 6.0f, 6.0f);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 30, 80, 80, p);
    cevg_paint_set_color(p, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 120, oy + 100, 80, 80, p);

    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p104_layer_transform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P104");

    /* Layer with transform applied before save_layer */
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 60, oy + 60);
    cevg_canvas_rotate(cv, 15.0f);

    float bounds[4] = { -50, -50, 50, 50 };
    cevg_canvas_save_layer(cv, 0.8f, bounds, NULL);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, -40, -40, 80, 80, p);

    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
}

static void p105_filter_clear(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P105");

    /* Set blur then clear — should render without blur */
    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_blur(lp, 10.0f, 10.0f);
    cevg_paint_clear_filter(lp);  /* clear the blur */
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 40, oy + 50, 140, 120, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p106_layer_multiple(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P106");

    /* Three separate layers, each with different alpha */
    float b1[4] = { ox + 16, oy + 30, ox + 100, oy + 120 };
    cevg_canvas_save_layer(cv, 0.5f, b1, NULL);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 84, 90, p);
    cevg_canvas_restore_layer(cv);

    float b2[4] = { ox + 70, oy + 60, ox + 160, oy + 150 };
    cevg_canvas_save_layer(cv, 0.7f, b2, NULL);
    cevg_paint_set_color(p, 0.0f, 1.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 70, oy + 60, 90, 90, p);
    cevg_canvas_restore_layer(cv);

    float b3[4] = { ox + 120, oy + 90, ox + 216, oy + 200 };
    cevg_canvas_save_layer(cv, 0.9f, b3, NULL);
    cevg_paint_set_color(p, 0.0f, 0.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 120, oy + 90, 96, 110, p);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
}

static void p107_backdrop_blur_frosted(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P107");

    /* Colorful background */
    uint32_t colors[] = { 0xFFFF4444, 0xFF4444FF, 0xFF44FF44 };
    CevgPaint* bg = cevg_paint_create();
    cevg_paint_set_linear_gradient(bg, ox + 16, oy + 20, ox + 216, oy + 20,
                                   colors, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 20, 200, 180, bg);

    /* Frosted glass: backdrop blur + semi-transparent white layer */
    float bounds[4] = { ox + 40, oy + 50, ox + 190, oy + 170 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, 12.0f, 12.0f);
    cevg_canvas_save_layer(cv, 0.6f, bounds, lp);

    /* Fill white inside layer */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 40, oy + 50, 150, 120, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(bg);
    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p108_layer_no_bounds(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P108");

    /* Layer with NULL bounds (full surface) */
    cevg_canvas_save_layer(cv, 0.6f, NULL, NULL);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 30, oy + 40, 160, 150, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
}

/* ================================================================== */
/* P109-P116: Image Drawing                                           */
/* ================================================================== */

/* Generate a 64x64 test image: red-to-green horizontal gradient with
 * a blue diagonal cross pattern. RGBA non-premultiplied. */
static CevgImage* make_test_image(void) {
    const int SZ = 64;
    static unsigned char pixels[64 * 64 * 4];
    static int generated = 0;
    if (!generated) {
        for (int y = 0; y < SZ; y++)
            for (int x = 0; x < SZ; x++) {
                int idx = (y * SZ + x) * 4;
                pixels[idx + 0] = (unsigned char)(255 - x * 255 / (SZ - 1));
                pixels[idx + 1] = (unsigned char)(x * 255 / (SZ - 1));
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 255;
                /* Blue cross: center row and center column */
                if (y == SZ / 2 || x == SZ / 2) {
                    pixels[idx + 2] = 255;
                    pixels[idx + 0] = 0;
                    pixels[idx + 1] = 0;
                }
            }
        generated = 1;
    }
    return cevg_image_create_from_pixels(pixels, SZ, SZ);
}

/* Generate a 16x16 tiny test image: 4 colored quadrants. */
static CevgImage* make_tiny_image(void) {
    const int SZ = 16;
    static unsigned char pixels[16 * 16 * 4];
    static int generated = 0;
    if (!generated) {
        for (int y = 0; y < SZ; y++)
            for (int x = 0; x < SZ; x++) {
                int idx = (y * SZ + x) * 4;
                /* Top-left: red, Top-right: green, Bottom-left: blue, Bottom-right: yellow */
                if (y < SZ / 2) {
                    if (x < SZ / 2) {
                        pixels[idx + 0] = 255; pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;   pixels[idx + 3] = 255;
                    } else {
                        pixels[idx + 0] = 0;   pixels[idx + 1] = 255;
                        pixels[idx + 2] = 0;   pixels[idx + 3] = 255;
                    }
                } else {
                    if (x < SZ / 2) {
                        pixels[idx + 0] = 0;   pixels[idx + 1] = 0;
                        pixels[idx + 2] = 255; pixels[idx + 3] = 255;
                    } else {
                        pixels[idx + 0] = 255; pixels[idx + 1] = 255;
                        pixels[idx + 2] = 0;   pixels[idx + 3] = 255;
                    }
                }
            }
        generated = 1;
    }
    return cevg_image_create_from_pixels(pixels, SZ, SZ);
}

static void p109_image_draw(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P109");

    CevgImage* img = make_test_image();
    if (!img) return;

    CevgPaint* p = cevg_paint_create();
    cevg_canvas_draw_image(cv, img, ox + 16, oy + 30, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p110_image_rect(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P110");

    CevgImage* img = make_test_image();
    if (!img) return;

    /* Source: top-left 32x32 quadrant, dest: 200x200 scaled up */
    float src[4] = { 0, 0, 32, 32 };
    float dst[4] = { ox + 16, oy + 30, 200, 180 };

    CevgPaint* p = cevg_paint_create();
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p111_image_scale_up(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P111");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    /* 16x16 image scaled to 160x160 */
    float src[4] = { 0, 0, 16, 16 };
    float dst[4] = { ox + 16, oy + 30, 160, 160 };

    CevgPaint* p = cevg_paint_create();
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p112_image_scale_down(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 6;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P112");

    CevgImage* img = make_test_image();
    if (!img) return;

    /* 64x64 image scaled down to 40x40 */
    float src[4] = { 0, 0, 64, 64 };
    float dst[4] = { ox + 40, oy + 50, 40, 40 };

    CevgPaint* p = cevg_paint_create();
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p113_image_nine(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P113");

    CevgImage* img = make_test_image();
    if (!img) return;

    /* Nine-patch: center region is the inner 32x32 area (LTRB) */
    float center[4] = { 16, 16, 48, 48 };
    float dst[4] = { ox + 16, oy + 30, 200, 180 };

    CevgPaint* p = cevg_paint_create();
    cevg_canvas_draw_image_nine(cv, img, center, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p114_image_filter_nearest(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P114");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    float src[4] = { 0, 0, 16, 16 };
    float dst[4] = { ox + 16, oy + 30, 160, 160 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_filter_quality(p, kCevgFilterQuality_Nearest);
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p115_image_filter_linear(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P115");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    float src[4] = { 0, 0, 16, 16 };
    float dst[4] = { ox + 16, oy + 30, 160, 160 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_filter_quality(p, kCevgFilterQuality_Linear);
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p116_image_filter_high(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P116");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    float src[4] = { 0, 0, 16, 16 };
    float dst[4] = { ox + 16, oy + 30, 160, 160 };

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_filter_quality(p, kCevgFilterQuality_High);
    cevg_canvas_draw_image_rect(cv, img, src, dst, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

/* ================================================================== */
/* P117-P120: Image Shader                                            */
/* ================================================================== */

static void p117_image_shader_clamp(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P117");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_image_shader(p, img, kCevgTile_Clamp, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 180, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p118_image_shader_repeat(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P118");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_image_shader(p, img, kCevgTile_Repeat, kCevgTile_Repeat);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 180, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p119_image_shader_mirror(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P119");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_image_shader(p, img, kCevgTile_Mirror, kCevgTile_Mirror);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 180, p);
    cevg_paint_destroy(p);
    cevg_image_destroy(img);
}

static void p120_image_shader_transform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P120");

    CevgImage* img = make_tiny_image();
    if (!img) return;

    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + CONTENT_W * 0.5f, oy + CONTENT_H * 0.5f);
    cevg_canvas_rotate(cv, 30.0f);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_image_shader(p, img, kCevgTile_Repeat, kCevgTile_Repeat);
    cevg_canvas_draw_rect(cv, -80, -60, 160, 120, p);
    cevg_paint_destroy(p);

    cevg_canvas_restore(cv);
    cevg_image_destroy(img);
}

/* ================================================================== */
/* P121-P128: Text Rendering                                          */
/* ================================================================== */

static void p121_text_basic(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P121");

    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make("Hello Cevg", 12, face, 20.0f, kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p122_text_color(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P122");

    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make("Colorful", 8, face, 24.0f, kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);  /* #FF4444 */
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p123_text_alpha(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P123");

    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make("Alpha", 5, face, 24.0f, kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 0.4f);
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p124_text_small(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P124");

    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make("Small 10px", 10, face, 10.0f, kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p125_text_emoji_cjk_en(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P125");

    if (!face) return;

    CevgPaint* p = cevg_paint_create();
    float y = oy + 50;

    /* Line 1: Emoji */
    const char* emoji = "\xf0\x9f\x98\x80\xf0\x9f\x91\x8b\xf0\x9f\x8e\x89\xf0\x9f\x92\x96";
    CevgTextBlob* b1 = cevg_text_blob_make(emoji, (int)strlen(emoji), face, 22.0f, kCevgDir_LTR);
    if (b1) {
        float tw = cevg_text_blob_get_width(b1);
        cevg_canvas_draw_text_blob(cv, b1, ox + (CONTENT_W - tw) * 0.5f, y, p);
        cevg_text_blob_destroy(b1);
    }
    y += 40;

    /* Line 2: Chinese */
    cevg_paint_set_color(p, 0.4f, 0.85f, 1.0f, 1.0f);
    const char* cn = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81";
    CevgTextBlob* b2 = cevg_text_blob_make(cn, (int)strlen(cn), face, 20.0f, kCevgDir_LTR);
    if (b2) {
        float tw = cevg_text_blob_get_width(b2);
        cevg_canvas_draw_text_blob(cv, b2, ox + (CONTENT_W - tw) * 0.5f, y, p);
        cevg_text_blob_destroy(b2);
    }
    y += 36;

    /* Line 3: English */
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    const char* en = "Hello FreeType!";
    CevgTextBlob* b3 = cevg_text_blob_make(en, (int)strlen(en), face, 20.0f, kCevgDir_LTR);
    if (b3) {
        float tw = cevg_text_blob_get_width(b3);
        cevg_canvas_draw_text_blob(cv, b3, ox + (CONTENT_W - tw) * 0.5f, y, p);
        cevg_text_blob_destroy(b3);
    }
    y += 36;

    /* Line 4: Mixed emoji + CJK + EN */
    cevg_paint_set_color(p, 1.0f, 0.9f, 0.3f, 1.0f);
    const char* mixed = "Hi\xf0\x9f\x91\x8b\xe4\xb8\xad\xe6\x96\x87" "FreeType";
    CevgTextBlob* b4 = cevg_text_blob_make(mixed, (int)strlen(mixed), face, 18.0f, kCevgDir_LTR);
    if (b4) {
        float tw = cevg_text_blob_get_width(b4);
        cevg_canvas_draw_text_blob(cv, b4, ox + (CONTENT_W - tw) * 0.5f, y, p);
        cevg_text_blob_destroy(b4);
    }

    cevg_paint_destroy(p);
}

static void p126_text_cjk(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P126");

    if (!face) return;
    /* Try CJK text — may not render if font lacks CJK glyphs */
    CevgTextBlob* blob = cevg_text_blob_make("\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c", 12, face, 24.0f, kCevgDir_LTR);
    if (!blob) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p127_text_blend(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P127");

    if (!face) return;

    /* Red rect behind */
    CevgPaint* pr = cevg_paint_create();
    cevg_paint_set_color(pr, 1.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 60, 200, 100, pr);

    /* Yellow text with Screen blend */
    CevgTextBlob* blob = cevg_text_blob_make("Blend", 5, face, 28.0f, kCevgDir_LTR);
    if (!blob) { cevg_paint_destroy(pr); return; }

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 0.0f, 1.0f);
    cevg_paint_set_blend_mode(p, kCevgBlendMode_Screen);
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);

    cevg_paint_destroy(pr);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

static void p128_text_on_gradient(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 7;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P128");

    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make("GRADIENT", 8, face, 22.0f, kCevgDir_LTR);
    if (!blob) return;

    /* Use linear gradient as text fill */
    uint32_t colors[] = { 0xFFFF0000, 0xFF0000FF };  /* red -> blue */
    CevgPaint* p = cevg_paint_create();
    float tw = cevg_text_blob_get_width(blob);
    float tx = ox + (CONTENT_W - tw) * 0.5f;
    cevg_paint_set_linear_gradient(p, tx, oy + 80, tx + tw, oy + 80,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_text_blob(cv, blob, tx, oy + 100, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

/* ================================================================== */
/* P129-P132: Display List                                             */
/* ================================================================== */

static void p129_display_list_record_replay(CevgCanvas* cv, CevgTypeface* face) {
    int col = 0, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P129");
    (void)face;

    /* Record a red rect + blue circle into a display list, then replay it */
    CevgContext* ctx = cevg_test_ctx();
    CevgCanvas* rec = cevg_display_list_record_begin(ctx, 200, 200);
    if (!rec) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.267f, 0.267f, 1.0f);  /* red rect */
    cevg_canvas_draw_rect(rec, 10, 10, 80, 60, p);
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);  /* blue circle */
    cevg_canvas_draw_circle(rec, 130, 80, 50, p);
    cevg_paint_destroy(p);

    CevgDisplayList* dl = cevg_display_list_record_end(rec);
    if (!dl) return;

    /* Replay at cell origin with identity matrix */
    float id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 16, oy + 20);
    cevg_canvas_draw_display_list(cv, dl, id);
    cevg_canvas_restore(cv);

    cevg_display_list_destroy(dl);
}

static void p130_display_list_with_transform(CevgCanvas* cv, CevgTypeface* face) {
    int col = 1, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P130");
    (void)face;

    /* Record a green star path, replay with 0.8x scale transform */
    CevgContext* ctx = cevg_test_ctx();
    CevgCanvas* rec = cevg_display_list_record_begin(ctx, 200, 200);
    if (!rec) return;

    float cx = 100.0f, cy = 100.0f;
    float outer = 80.0f, inner = 32.0f;
    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float a = (float)(-M_PI / 2.0) + i * (float)M_PI / 5.0f;
        float r = (i % 2 == 0) ? outer : inner;
        float px = cx + r * cosf(a);
        float py = cy + r * sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);  /* green star */
    cevg_canvas_draw_path(rec, path, p);
    cevg_paint_destroy(p);
    cevg_path_destroy(path);

    CevgDisplayList* dl = cevg_display_list_record_end(rec);
    if (!dl) return;

    /* 0.8x scale matrix */
    float mat[9] = { 0.8f,0,0, 0,0.8f,0, 0,0,1 };
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 30, oy + 30);
    cevg_canvas_draw_display_list(cv, dl, mat);
    cevg_canvas_restore(cv);

    cevg_display_list_destroy(dl);
}

static void p131_display_list_replay_multiple(CevgCanvas* cv, CevgTypeface* face) {
    int col = 2, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P131");
    (void)face;

    /* Record a small yellow circle, replay it 4 times at different positions */
    CevgContext* ctx = cevg_test_ctx();
    CevgCanvas* rec = cevg_display_list_record_begin(ctx, 40, 40);
    if (!rec) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 0.267f, 1.0f);  /* yellow circle */
    cevg_canvas_draw_circle(rec, 20, 20, 18, p);
    cevg_paint_destroy(p);

    CevgDisplayList* dl = cevg_display_list_record_end(rec);
    if (!dl) return;

    float id[9] = { 1,0,0, 0,1,0, 0,0,1 };

    /* Replay 4 times at different positions using translation matrices */
    float positions[4][2] = {
        { ox + 20,  oy + 30  },
        { ox + 120, oy + 30  },
        { ox + 20,  oy + 130 },
        { ox + 120, oy + 130 },
    };
    for (int i = 0; i < 4; i++) {
        float mat[9] = { 1,0,0, 0,1,0, positions[i][0],positions[i][1],1 };
        cevg_canvas_draw_display_list(cv, dl, mat);
    }

    cevg_display_list_destroy(dl);
}

static void p132_display_list_with_clip(CevgCanvas* cv, CevgTypeface* face) {
    int col = 3, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P132");
    (void)face;

    /* Record overlapping rects, replay inside a circular clip */
    CevgContext* ctx = cevg_test_ctx();
    CevgCanvas* rec = cevg_display_list_record_begin(ctx, 200, 200);
    if (!rec) return;

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.0f, 1.0f);  /* orange rect */
    cevg_canvas_draw_rect(rec, 0, 0, 120, 120, p);
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);  /* blue rect */
    cevg_canvas_draw_rect(rec, 60, 60, 140, 140, p);
    cevg_paint_destroy(p);

    CevgDisplayList* dl = cevg_display_list_record_end(rec);
    if (!dl) return;

    /* Circular clip */
    float ccx = ox + CONTENT_W * 0.5f;
    float ccy = oy + CONTENT_H * 0.5f;
    float r = 80.0f;
    int segs = 48;

    CevgPath* clip_path = cevg_path_create();
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = ccx + r * cosf(a);
        float py = ccy + r * sinf(a);
        if (i == 0) cevg_path_move_to(clip_path, px, py);
        else        cevg_path_line_to(clip_path, px, py);
    }
    cevg_path_close(clip_path);

    cevg_canvas_save(cv);
    cevg_canvas_clip_path(cv, clip_path);

    float id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox + 16, oy + 20);
    cevg_canvas_draw_display_list(cv, dl, id);
    cevg_canvas_restore(cv);

    cevg_canvas_restore(cv);

    cevg_path_destroy(clip_path);
    cevg_display_list_destroy(dl);
}

/* ================================================================== */
/* P133-P144: Comprehensive Scenes                                    */
/* ================================================================== */

static void p133_scene_card(CevgCanvas* cv, CevgTypeface* face) {
    int col = 4, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P133");

    /* Dark card with title and separator */
    CevgPaint* p = cevg_paint_create();

    /* Card background */
    cevg_paint_set_color(p, 0.15f, 0.15f, 0.2f, 1.0f);
    cevg_canvas_draw_round_rect(cv, ox + 20, oy + 30, 192, 180, 12, 12, p);

    /* Title text */
    if (face) {
        CevgTextBlob* blob = cevg_text_blob_make("Card Title", 10, face, 14.0f, kCevgDir_LTR);
        if (blob) {
            cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
            cevg_canvas_draw_text_blob(cv, blob, ox + 36, oy + 60, p);
            cevg_text_blob_destroy(blob);
        }
    }

    /* Separator line */
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 1.0f);
    cevg_paint_set_color(p, 0.4f, 0.4f, 0.5f, 1.0f);
    cevg_canvas_draw_line(cv, ox + 36, oy + 80, ox + 196, oy + 80, p);

    /* Body text */
    if (face) {
        CevgTextBlob* blob = cevg_text_blob_make("Content here", 12, face, 11.0f, kCevgDir_LTR);
        if (blob) {
            cevg_paint_set_style(p, kCevgStyle_Fill);
            cevg_paint_set_color(p, 0.7f, 0.7f, 0.8f, 1.0f);
            cevg_canvas_draw_text_blob(cv, blob, ox + 36, oy + 100, p);
            cevg_text_blob_destroy(blob);
        }
    }

    cevg_paint_destroy(p);
}

static void p134_scene_button(CevgCanvas* cv, CevgTypeface* face) {
    int col = 5, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P134");

    /* Blue gradient button */
    uint32_t colors[] = { 0xFF4488FF, 0xFF2244CC };  /* light blue -> dark blue */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_linear_gradient(p, ox + 30, oy + 70, ox + 30, oy + 140,
                                   colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_round_rect(cv, ox + 30, oy + 70, 172, 70, 16, 16, p);

    /* Button text */
    if (face) {
        CevgTextBlob* blob = cevg_text_blob_make("Button", 6, face, 18.0f, kCevgDir_LTR);
        if (blob) {
            cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
            float tw = cevg_text_blob_get_width(blob);
            float tx = ox + 30 + (172 - tw) * 0.5f;
            float ty = oy + 70 + (70 - 18) * 0.5f;
            cevg_canvas_draw_text_blob(cv, blob, tx, ty, p);
            cevg_text_blob_destroy(blob);
        }
    }

    cevg_paint_destroy(p);
}

static void p135_scene_shadow(CevgCanvas* cv, CevgTypeface* face) {
    int col = 6, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P135");

    /* Card with drop shadow */
    float bounds[4] = { ox + 16, oy + 16, ox + 216, oy + 216 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_drop_shadow(lp, 4.0f, 4.0f, 6.0f, 0xFF000000);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    cevg_canvas_draw_round_rect(cv, ox + 30, oy + 40, 170, 150, 12, 12, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p136_scene_blur_card(CevgCanvas* cv, CevgTypeface* face) {
    int col = 7, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P136");

    /* Colorful stripes background */
    CevgPaint* p = cevg_paint_create();
    float stripe_h = 30;
    uint32_t stripe_colors[] = { 0xFFFF4444, 0xFF44FF44, 0xFF4444FF, 0xFFFFFF44, 0xFFFF44FF, 0xFF44FFFF, 0xFFFF8844, 0xFF8844FF };
    for (int i = 0; i < 8; i++) {
        float r = ((stripe_colors[i] >> 16) & 0xFF) / 255.0f;
        float g = ((stripe_colors[i] >> 8) & 0xFF) / 255.0f;
        float b = (stripe_colors[i] & 0xFF) / 255.0f;
        cevg_paint_set_color(p, r, g, b, 1.0f);
        cevg_canvas_draw_rect(cv, ox + 16, oy + 20 + i * stripe_h, 200, stripe_h, p);
    }

    /* Frosted glass card */
    float bounds[4] = { ox + 30, oy + 50, ox + 200, oy + 180 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, 8.0f, 8.0f);
    cevg_canvas_save_layer(cv, 0.7f, bounds, lp);

    cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
    cevg_canvas_draw_round_rect(cv, ox + 30, oy + 50, 170, 130, 12, 12, p);

    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

static void p137_scene_gradient_bar(CevgCanvas* cv, CevgTypeface* face) {
    int col = 8, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P137");

    /* Progress bar: gray border + gradient fill */
    CevgPaint* p = cevg_paint_create();

    /* Border */
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 2.0f);
    cevg_paint_set_color(p, 0.5f, 0.5f, 0.5f, 1.0f);
    cevg_canvas_draw_round_rect(cv, ox + 20, oy + 90, 192, 30, 8, 8, p);

    /* Gradient fill (green -> yellow -> red) */
    uint32_t colors[] = { 0xFF44FF44, 0xFFFFFF44, 0xFFFF4444 };
    cevg_paint_set_style(p, kCevgStyle_Fill);
    cevg_paint_set_linear_gradient(p, ox + 22, oy + 92, ox + 210, oy + 92,
                                   colors, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_round_rect(cv, ox + 22, oy + 92, 188, 26, 6, 6, p);

    cevg_paint_destroy(p);
}

static void p138_scene_icon_grid(CevgCanvas* cv, CevgTypeface* face) {
    int col = 9, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P138");

    /* 3x3 grid of colored circles */
    uint32_t icon_colors[] = {
        0xFFFF4444, 0xFF44FF44, 0xFF4444FF,
        0xFFFFFF44, 0xFFFF44FF, 0xFF44FFFF,
        0xFFFF8844, 0xFF8844FF, 0xFF44FF88
    };

    CevgPaint* p = cevg_paint_create();
    float spacing = 65;
    float start_x = ox + 30;
    float start_y = oy + 40;

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int idx = r * 3 + c;
            float cr = ((icon_colors[idx] >> 16) & 0xFF) / 255.0f;
            float cg = ((icon_colors[idx] >> 8) & 0xFF) / 255.0f;
            float cb = (icon_colors[idx] & 0xFF) / 255.0f;
            cevg_paint_set_color(p, cr, cg, cb, 1.0f);
            cevg_canvas_draw_circle(cv, start_x + c * spacing + 25,
                                    start_y + r * spacing + 25, 22, p);
        }
    }

    cevg_paint_destroy(p);
}

static void p139_scene_chart(CevgCanvas* cv, CevgTypeface* face) {
    int col = 10, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P139");

    /* Bar chart: 5 bars of different heights */
    float heights[] = { 60, 100, 80, 130, 50 };
    uint32_t bar_colors[] = { 0xFF4488FF, 0xFF44CC44, 0xFFFF8844, 0xFFFF4444, 0xFF8844FF };
    float bar_w = 28;
    float gap = 8;
    float base_y = oy + 200;

    CevgPaint* p = cevg_paint_create();
    for (int i = 0; i < 5; i++) {
        float r = ((bar_colors[i] >> 16) & 0xFF) / 255.0f;
        float g = ((bar_colors[i] >> 8) & 0xFF) / 255.0f;
        float b = (bar_colors[i] & 0xFF) / 255.0f;
        cevg_paint_set_color(p, r, g, b, 1.0f);
        float bx = ox + 20 + i * (bar_w + gap);
        cevg_canvas_draw_rect(cv, bx, base_y - heights[i], bar_w, heights[i], p);
    }

    /* Baseline */
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 1.0f);
    cevg_paint_set_color(p, 0.5f, 0.5f, 0.6f, 1.0f);
    cevg_canvas_draw_line(cv, ox + 16, base_y, ox + 216, base_y, p);

    cevg_paint_destroy(p);
}

static void p140_scene_overlay(CevgCanvas* cv, CevgTypeface* face) {
    int col = 11, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P140");

    /* Content underneath */
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 20, oy + 40, 80, 80, p);
    cevg_paint_set_color(p, 0.0f, 0.533f, 1.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 120, oy + 40, 80, 80, p);
    cevg_paint_set_color(p, 0.267f, 1.0f, 0.267f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 60, oy + 120, 100, 80, p);

    /* Semi-transparent dark overlay */
    cevg_canvas_save_layer(cv, 0.6f, NULL, NULL);
    cevg_paint_set_color(p, 0.0f, 0.0f, 0.0f, 1.0f);
    cevg_canvas_draw_rect(cv, ox + 16, oy + 30, 200, 190, p);
    cevg_canvas_restore_layer(cv);

    cevg_paint_destroy(p);
}

static void p141_scene_clip_art(CevgCanvas* cv, CevgTypeface* face) {
    int col = 12, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P141");

    /* Star clip with radial gradient fill */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f;
    float outer = 90.0f, inner = 36.0f;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float a = (float)(-M_PI / 2.0) + i * (float)M_PI / 5.0f;
        float r = (i % 2 == 0) ? outer : inner;
        float px = cx + r * cosf(a);
        float py = cy + r * sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    cevg_canvas_save(cv);
    cevg_canvas_clip_path(cv, path);

    /* Radial gradient fill */
    uint32_t colors[] = { 0xFFFF4444, 0xFFFFFF44, 0xFF4444FF };
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_radial_gradient(p, cx, cy, 90.0f, colors, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);

    cevg_canvas_restore(cv);

    cevg_paint_destroy(p);
    cevg_path_destroy(path);
}

static void p142_scene_text_block(CevgCanvas* cv, CevgTypeface* face) {
    int col = 13, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P142");

    if (!face) return;

    CevgPaint* p = cevg_paint_create();

    /* Line 1: large white */
    CevgTextBlob* b1 = cevg_text_blob_make("Heading", 7, face, 18.0f, kCevgDir_LTR);
    if (b1) {
        cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
        cevg_canvas_draw_text_blob(cv, b1, ox + 20, oy + 50, p);
        cevg_text_blob_destroy(b1);
    }

    /* Line 2: medium gray */
    CevgTextBlob* b2 = cevg_text_blob_make("Subheading text", 15, face, 13.0f, kCevgDir_LTR);
    if (b2) {
        cevg_paint_set_color(p, 0.7f, 0.7f, 0.8f, 1.0f);
        cevg_canvas_draw_text_blob(cv, b2, ox + 20, oy + 80, p);
        cevg_text_blob_destroy(b2);
    }

    /* Line 3: small dim */
    CevgTextBlob* b3 = cevg_text_blob_make("Body text goes here", 19, face, 11.0f, kCevgDir_LTR);
    if (b3) {
        cevg_paint_set_color(p, 0.5f, 0.5f, 0.6f, 1.0f);
        cevg_canvas_draw_text_blob(cv, b3, ox + 20, oy + 105, p);
        cevg_text_blob_destroy(b3);
    }

    cevg_paint_destroy(p);
}

static void p143_scene_image_mask(CevgCanvas* cv, CevgTypeface* face) {
    int col = 14, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P143");

    /* Circular clip + image = circular picture */
    float cx = ox + CONTENT_W * 0.5f;
    float cy = oy + CONTENT_H * 0.5f + 10;
    float r = 80.0f;
    int segs = 48;

    CevgPath* path = cevg_path_create();
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = cx + r * cosf(a);
        float py = cy + r * sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);

    cevg_canvas_save(cv);
    cevg_canvas_clip_path(cv, path);

    CevgImage* img = make_test_image();
    if (img) {
        /* Scale image to fill circle area */
        float src[4] = { 0, 0, 64, 64 };
        float dst[4] = { cx - r, cy - r, r * 2, r * 2 };
        CevgPaint* p = cevg_paint_create();
        cevg_canvas_draw_image_rect(cv, img, src, dst, p);
        cevg_paint_destroy(p);
        cevg_image_destroy(img);
    }

    cevg_canvas_restore(cv);
    cevg_path_destroy(path);
}

static void p144_scene_full(CevgCanvas* cv, CevgTypeface* face) {
    int col = 15, row = 8;
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    draw_label(cv, face, col, row, "P144");

    CevgPaint* p = cevg_paint_create();

    /* Background gradient */
    uint32_t bg_colors[] = { 0xFF1A1A3E, 0xFF2A2A5E };
    cevg_paint_set_linear_gradient(p, ox, oy, ox + CONTENT_W, oy + CONTENT_H,
                                   bg_colors, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox, oy, (float)CONTENT_W, (float)CONTENT_H, p);

    /* Card with shadow */
    float bounds[4] = { ox + 20, oy + 30, ox + 210, oy + 200 };
    CevgPaint* lp = cevg_paint_create();
    cevg_paint_set_drop_shadow(lp, 3.0f, 3.0f, 5.0f, 0xFF000000);
    cevg_canvas_save_layer(cv, 1.0f, bounds, lp);

    cevg_paint_set_color(p, 0.2f, 0.2f, 0.3f, 1.0f);
    cevg_canvas_draw_round_rect(cv, ox + 25, oy + 35, 180, 160, 10, 10, p);

    cevg_canvas_restore_layer(cv);

    /* Small icon circles */
    cevg_paint_set_color(p, 0.267f, 0.533f, 1.0f, 1.0f);
    cevg_canvas_draw_circle(cv, ox + 50, oy + 70, 12, p);
    cevg_paint_set_color(p, 1.0f, 0.533f, 0.267f, 1.0f);
    cevg_canvas_draw_circle(cv, ox + 80, oy + 70, 12, p);

    /* Separator line */
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 1.0f);
    cevg_paint_set_color(p, 0.4f, 0.4f, 0.5f, 1.0f);
    cevg_canvas_draw_line(cv, ox + 35, oy + 95, ox + 195, oy + 95, p);

    /* Text */
    if (face) {
        CevgTextBlob* blob = cevg_text_blob_make("Full Scene", 10, face, 13.0f, kCevgDir_LTR);
        if (blob) {
            cevg_paint_set_style(p, kCevgStyle_Fill);
            cevg_paint_set_color(p, 1.0f, 1.0f, 1.0f, 1.0f);
            cevg_canvas_draw_text_blob(cv, blob, ox + 40, oy + 120, p);
            cevg_text_blob_destroy(blob);
        }
    }

    cevg_paint_destroy(p);
    cevg_paint_destroy(lp);
}

/* ================================================================== */
/* Cell filter: --only / --skip command-line selection                */
/* ================================================================== */

#define MAX_TESTS 144

typedef struct {
    const char* id;     /* "P01", "P02", ... */
    void (*fn)(CevgCanvas*, CevgTypeface*);
} TestEntry;

/* Bitmask: 1 = enabled, 0 = disabled */
static uint32_t g_enabled[(MAX_TESTS + 31) / 32];

static void enable_all(void) {
    memset(g_enabled, 0xFF, sizeof(g_enabled));
}

static void enable_test(int idx) {
    if (idx >= 0 && idx < MAX_TESTS)
        g_enabled[idx / 32] |= (1u << (idx % 32));
}

static void disable_test(int idx) {
    if (idx >= 0 && idx < MAX_TESTS)
        g_enabled[idx / 32] &= ~(1u << (idx % 32));
}

static bool is_enabled(int idx) {
    if (idx < 0 || idx >= MAX_TESTS) return false;
    return (g_enabled[idx / 32] >> (idx % 32)) & 1u;
}

/* Parse "P01" -> 0, "P12" -> 11, etc. Returns -1 on failure. */
static int parse_test_id(const char* s) {
    if (!s) return -1;
    int n;
    if (s[0] == 'P' || s[0] == 'p') {
        n = atoi(s + 1);
    } else {
        n = atoi(s);
    }
    if (n < 1 || n > MAX_TESTS) return -1;
    return n - 1;
}

/* Parse comma-separated test IDs like "P01,P03,P05" */
static void parse_list(const char* list, int* out, int* count, int max) {
    *count = 0;
    size_t len = strlen(list);
    char* buf = (char*)malloc(len + 1);
    if (!buf) return;
    memcpy(buf, list, len + 1);
    char* tok = strtok(buf, ",");
    while (tok && *count < max) {
        int idx = parse_test_id(tok);
        if (idx >= 0) out[(*count)++] = idx;
        tok = strtok(NULL, ",");
    }
    free(buf);
}

static void print_usage(const char* prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  --only P01,P03,...   Run only these tests\n");
    printf("  --skip P02,P05,...   Skip these tests\n");
    printf("  --list               List all test IDs and exit\n");
    printf("  --help               Show this help\n");
    printf("\nExamples:\n");
    printf("  %s --only P01,P06        Run only P01 and P06\n", prog);
    printf("  %s --skip P03,P07,P11    Run all except P03, P07, P11\n", prog);
}

/* ================================================================== */
/* Test table                                                         */
/* ================================================================== */

static const TestEntry g_tests[] = {
    { "P01", p01_fill_rect },
    { "P02", p02_fill_rect_alpha },
    { "P03", p03_stroke_rect },
    { "P04", p04_fill_round_rect },
    { "P05", p05_stroke_round_rect },
    { "P06", p06_fill_circle },
    { "P07", p07_stroke_circle },
    { "P08", p08_fill_oval },
    { "P09", p09_stroke_oval },
    { "P10", p10_draw_line },
    { "P11", p11_draw_line_thick },
    { "P12", p12_draw_paint },
    { "P13", p13_path_triangle },
    { "P14", p14_path_pentagon },
    { "P15", p15_path_star },
    { "P16", p16_path_star_eodd },
    { "P17", p17_path_quad_bezier },
    { "P18", p18_path_cubic_bezier },
    { "P19", p19_path_stroke },
    { "P20", p20_path_multi_contour },
    { "P21", p21_path_concave },
    { "P22", p22_path_complex },
    { "P23", p23_path_dash },
    { "P24", p24_path_rewind },
    { "P25", p25_cap_butt },
    { "P26", p26_cap_round },
    { "P27", p27_cap_square },
    { "P28", p28_join_miter },
    { "P29", p29_join_round },
    { "P30", p30_join_bevel },
    { "P31", p31_miter_limit },
    { "P32", p32_stroke_hairline },
    { "P33", p33_dash_even },
    { "P34", p34_dash_uneven },
    { "P35", p35_dash_phase },
    { "P36", p36_dash_round_cap },
    { "P37", p37_translate },
    { "P38", p38_scale_uniform },
    { "P39", p39_scale_nonuniform },
    { "P40", p40_rotate_45 },
    { "P41", p41_rotate_90 },
    { "P42", p42_skew },
    { "P43", p43_concat_matrix },
    { "P44", p44_reset_matrix },
    { "P45", p45_save_restore },
    { "P46", p46_nested_transform },
    { "P47", p47_transform_line },
    { "P48", p48_transform_path },
    { "P49", p49_clip_rect },
    { "P50", p50_clip_rect_nested },
    { "P51", p51_clip_path },
    { "P52", p52_clip_path_star },
    { "P53", p53_clip_transform },
    { "P54", p54_clip_save_restore },
    { "P55", p55_clip_multiple },
    { "P56", p56_clip_empty },
    { "P57", p57_blend_srcover },
    { "P58", p58_blend_src },
    { "P59", p59_blend_dst },
    { "P60", p60_blend_clear },
    { "P61", p61_blend_src_in },
    { "P62", p62_blend_dst_in },
    { "P63", p63_blend_src_out },
    { "P64", p64_blend_dst_out },
    { "P65", p65_blend_src_atop },
    { "P66", p66_blend_xor },
    { "P67", p67_blend_screen },
    { "P68", p68_blend_multiply },
    { "P69", p69_blend_overlay },
    { "P70", p70_blend_darken },
    { "P71", p71_blend_lighten },
    { "P72", p72_blend_difference },
    { "P73", p73_grad_linear_2stop },
    { "P74", p74_grad_linear_3stop },
    { "P75", p75_grad_linear_vertical },
    { "P76", p76_grad_linear_diagonal },
    { "P77", p77_grad_linear_clamp },
    { "P78", p78_grad_linear_repeat },
    { "P79", p79_grad_linear_mirror },
    { "P80", p80_grad_linear_alpha },
    { "P81", p81_grad_radial_2stop },
    { "P82", p82_grad_radial_3stop },
    { "P83", p83_grad_radial_center },
    { "P84", p84_grad_radial_offset },
    { "P85", p85_grad_radial_clamp },
    { "P86", p86_grad_radial_repeat },
    { "P87", p87_grad_radial_mirror },
    { "P88", p88_grad_radial_alpha },
    { "P89", p89_layer_alpha },
    { "P90", p90_layer_blend },
    { "P91", p91_layer_nested },
    { "P92", p92_layer_empty },
    { "P93", p93_filter_blur },
    { "P94", p94_filter_blur_large },
    { "P95", p95_filter_drop_shadow },
    { "P96", p96_filter_drop_shadow_color },
    { "P97", p97_filter_color_matrix },
    { "P98", p98_filter_color_matrix_invert },
    { "P99", p99_filter_blur_and_shadow },
    { "P100", p100_backdrop_blur },
    { "P101", p101_backdrop_blur_alpha },
    { "P102", p102_backdrop_shadow },
    { "P103", p103_layer_clip },
    { "P104", p104_layer_transform },
    { "P105", p105_filter_clear },
    { "P106", p106_layer_multiple },
    { "P107", p107_backdrop_blur_frosted },
    { "P108", p108_layer_no_bounds },
    { "P109", p109_image_draw },
    { "P110", p110_image_rect },
    { "P111", p111_image_scale_up },
    { "P112", p112_image_scale_down },
    { "P113", p113_image_nine },
    { "P114", p114_image_filter_nearest },
    { "P115", p115_image_filter_linear },
    { "P116", p116_image_filter_high },
    { "P117", p117_image_shader_clamp },
    { "P118", p118_image_shader_repeat },
    { "P119", p119_image_shader_mirror },
    { "P120", p120_image_shader_transform },
    { "P121", p121_text_basic },
    { "P122", p122_text_color },
    { "P123", p123_text_alpha },
    { "P124", p124_text_small },
    { "P125", p125_text_emoji_cjk_en },
    { "P126", p126_text_cjk },
    { "P127", p127_text_blend },
    { "P128", p128_text_on_gradient },
    { "P129", p129_display_list_record_replay },
    { "P130", p130_display_list_with_transform },
    { "P131", p131_display_list_replay_multiple },
    { "P132", p132_display_list_with_clip },
    { "P133", p133_scene_card },
    { "P134", p134_scene_button },
    { "P135", p135_scene_shadow },
    { "P136", p136_scene_blur_card },
    { "P137", p137_scene_gradient_bar },
    { "P138", p138_scene_icon_grid },
    { "P139", p139_scene_chart },
    { "P140", p140_scene_overlay },
    { "P141", p141_scene_clip_art },
    { "P142", p142_scene_text_block },
    { "P143", p143_scene_image_mask },
    { "P144", p144_scene_full },
};
#define TEST_COUNT (sizeof(g_tests) / sizeof(g_tests[0]))

/* ================================================================== */
/* Main                                                                */
/* ================================================================== */

#ifndef CEVG_TEST_NO_MAIN
int main(int argc, char** argv) {
    printf("=== Cevg Rendering Test (P01-P144) ===\n");

    /* Parse command-line arguments */
    enable_all();

    bool has_only = false;
    int only_list[MAX_TESTS], only_count = 0;
    int skip_list[MAX_TESTS], skip_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--list") == 0) {
            for (int j = 0; j < (int)TEST_COUNT; j++)
                printf("  %s\n", g_tests[j].id);
            return 0;
        } else if (strcmp(argv[i], "--only") == 0 && i + 1 < argc) {
            has_only = true;
            parse_list(argv[++i], only_list, &only_count, MAX_TESTS);
        } else if (strcmp(argv[i], "--skip") == 0 && i + 1 < argc) {
            parse_list(argv[++i], skip_list, &skip_count, MAX_TESTS);
        }
    }

    /* Apply --only: disable everything, then enable only listed */
    if (has_only) {
        memset(g_enabled, 0, sizeof(g_enabled));
        for (int i = 0; i < only_count; i++)
            enable_test(only_list[i]);
    }

    /* Apply --skip: disable listed tests */
    for (int i = 0; i < skip_count; i++)
        disable_test(skip_list[i]);

    /* Print active tests */
    printf("Active tests:");
    int active_count = 0;
    for (int i = 0; i < (int)TEST_COUNT; i++) {
        if (is_enabled(i)) {
            printf(" %s", g_tests[i].id);
            active_count++;
        }
    }
    printf(" (%d/%d)\n", active_count, (int)TEST_COUNT);

    /* 1. Init Cevg context + surface */
    CevgContext* ctx = cevg_test_init(FB_W, FB_H);
    if (!ctx) {
        fprintf(stderr, "Failed to init Cevg context\n");
        return 1;
    }
    CevgSurface* surf = cevg_test_surface();
    CevgCanvas*  cv   = cevg_test_canvas();
    printf("Context: %s, surface: %dx%d\n",
           cevg_backend_name(),
           cevg_surface_get_width(surf),
           cevg_surface_get_height(surf));

    /* 2. Load font for labels */
    const char* fontSearchPaths[] = {
        "./fonts/DejaVuSans.ttf",
        "./DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        NULL
    };
    CevgTypeface* face = NULL;
    for (int i = 0; fontSearchPaths[i]; i++) {
        face = cevg_typeface_create_from_file(fontSearchPaths[i]);
        if (face) { printf("Font: %s\n", fontSearchPaths[i]); break; }
    }
    if (!face) {
        printf("WARNING: No font loaded, labels will be skipped\n");
    }

    /* 3. Clear to background color */
    cevg_canvas_clear(cv, BG_R, BG_G, BG_B, BG_A);

    /* 4. Draw cell borders */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            draw_cell_border(cv, c, r);
        }
    }

    /* 5. Draw enabled tests */
    for (int i = 0; i < (int)TEST_COUNT; i++) {
        if (is_enabled(i)) {
            g_tests[i].fn(cv, face);
        }
    }

    /* 6. Flush and save */
    cevg_canvas_flush(cv);
    cevg_test_save_png("test_cevg_render.png");
    printf("PNG saved: test_cevg_render.png (%dx%d)\n", FB_W, FB_H);

    /* 7. Cleanup */
    if (face) cevg_typeface_unref(face);
    cevg_test_shutdown();

    printf("Done!\n");
    return 0;
}
#endif /* CEVG_TEST_NO_MAIN */

/* Exported for benchmark use. Draws the full P01-P144 grid. */
void render_cevg_render_all(CevgCanvas* cv, CevgTypeface* face) {
    cevg_canvas_clear(cv, BG_R, BG_G, BG_B, BG_A);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            draw_cell_border(cv, c, r);
        }
    }
    for (int i = 0; i < (int)TEST_COUNT; i++) {
        g_tests[i].fn(cv, face);
    }
}
