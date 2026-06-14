/* =====================================================================
 * test_cevg_overlap.c — Cevg OVERLAP / COMPOSITING test suite
 * ---------------------------------------------------------------------
 * Companion to test_cevg_render.c. Where that suite draws ONE isolated
 * primitive per cell, this suite stresses STACKED / OVERLAPPING operations
 * inside each cell: multiple draws that composite on top of each other,
 * transparency stacking, group (layer) opacity, every blend mode over a
 * real backdrop, backdrop copy / frosted glass, clip + overlap, nested
 * clips, blur-over-content, drop-shadow stacks, color-matrix over mixed
 * content, transforms with overlap, and combined filter pipelines.
 *
 * Same 16x9 grid of 240x240 cells, same label/border helpers, same CLI
 * (--only / --skip / --list). Output: test_cevg_overlap.png
 *
 * Build identically to test_cevg_render (same harness, same backend
 * macro). Each O-test is named O001..O144.
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

/* ---- Grid layout constants (identical to render suite) ---- */
#define FB_W       3840
#define FB_H       2160
#define COLS       16
#define ROWS       9
#define CELL_W     240
#define CELL_H     240
#define PAD        4

#define BG_R  0.102f
#define BG_G  0.102f
#define BG_B  0.180f
#define BG_A  1.0f

#define CONTENT_W  (CELL_W - PAD * 2)
#define CONTENT_H  (CELL_H - PAD * 2)

static void cell_origin(int col, int row, float* ox, float* oy) {
    *ox = (float)(col * CELL_W + PAD);
    *oy = (float)(row * CELL_H + PAD);
}

static void draw_label(CevgCanvas* cv, CevgTypeface* face, int col, int row,
                       const char* text) {
    float ox, oy;
    cell_origin(col, row, &ox, &oy);
    if (!face) return;
    CevgTextBlob* blob = cevg_text_blob_make(text, strlen(text), face, 11.0f,
                                             kCevgDir_LTR);
    if (!blob) return;
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, 0.7f, 0.7f, 0.8f, 0.9f);
    cevg_canvas_draw_text_blob(cv, blob, ox + 2, oy + 2, p);
    cevg_paint_destroy(p);
    cevg_text_blob_destroy(blob);
}

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

/* ---- Shared helpers for overlap tests ---- */

/* Solid rect convenience */
static void rect(CevgCanvas* cv, float x, float y, float w, float h,
                 float r, float g, float b, float a) {
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, r, g, b, a);
    cevg_canvas_draw_rect(cv, x, y, w, h, p);
    cevg_paint_destroy(p);
}

/* Solid circle convenience */
static void circ(CevgCanvas* cv, float cx, float cy, float rad,
                 float r, float g, float b, float a) {
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_color(p, r, g, b, a);
    cevg_canvas_draw_circle(cv, cx, cy, rad, p);
    cevg_paint_destroy(p);
}

/* A "busy" colorful backdrop used by many compositing tests so the effect of
 * a blend / blur / clip / backdrop-copy is clearly visible. Fills the content
 * area of the cell at (ox,oy). */
static void busy_backdrop(CevgCanvas* cv, float ox, float oy) {
    rect(cv, ox,        oy,        CONTENT_W*0.5f, CONTENT_H*0.5f, 0.90f, 0.20f, 0.20f, 1.0f);
    rect(cv, ox+CONTENT_W*0.5f, oy, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.20f, 0.70f, 0.25f, 1.0f);
    rect(cv, ox,        oy+CONTENT_H*0.5f, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.20f, 0.35f, 0.90f, 1.0f);
    rect(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.95f, 0.80f, 0.10f, 1.0f);
    /* diagonal stripes on top to add high-frequency detail (good for blur) */
    CevgPaint* sp = cevg_paint_create();
    cevg_paint_set_color(sp, 1.0f, 1.0f, 1.0f, 0.18f);
    for (int i = -4; i < 12; i++) {
        cevg_canvas_draw_rect(cv, ox + i*22.0f, oy, 8.0f, CONTENT_H, sp);
    }
    cevg_paint_destroy(sp);
}

/* A small star path centered at (cx,cy) — used for clip-path overlap tests. */
static CevgPath* make_star(float cx, float cy, float outer, float inner) {
    CevgPath* path = cevg_path_create();
    for (int i = 0; i < 10; i++) {
        float a = (float)(-M_PI/2.0) + i * (float)M_PI/5.0f;
        float r = (i % 2 == 0) ? outer : inner;
        float px = cx + r*cosf(a), py = cy + r*sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);
    return path;
}

/* A circle path for clip tests. */
static CevgPath* make_circle_path(float cx, float cy, float r) {
    CevgPath* path = cevg_path_create();
    const int segs = 48;
    for (int i = 0; i < segs; i++) {
        float a = i * 2.0f * (float)M_PI / segs;
        float px = cx + r*cosf(a), py = cy + r*sinf(a);
        if (i == 0) cevg_path_move_to(path, px, py);
        else        cevg_path_line_to(path, px, py);
    }
    cevg_path_close(path);
    return path;
}

/* ================================================================== */
/* O001-O016 : TRANSPARENCY STACKING (per-shape alpha, overlap order)  */
/* ================================================================== */

/* O001 — two 50% rects overlapping: overlap is darker/more opaque */
static void o001(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O001");
    rect(cv, ox+30, oy+40, 110, 110, 1.0f,0.2f,0.2f, 0.5f);
    rect(cv, ox+90, oy+90, 110, 110, 0.2f,0.4f,1.0f, 0.5f);
}

/* O002 — three primary circles at 50%, classic additive-ish RGB overlap */
static void o002(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O002");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    circ(cv, cx,       cy-30, 55, 1.0f,0.1f,0.1f, 0.5f);
    circ(cv, cx-32,    cy+24, 55, 0.1f,1.0f,0.1f, 0.5f);
    circ(cv, cx+32,    cy+24, 55, 0.1f,0.1f,1.0f, 0.5f);
}

/* O003 — stack of 6 translucent rects, increasing opacity buildup */
static void o003(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O003");
    for (int i=0;i<6;i++)
        rect(cv, ox+20+i*8, oy+30+i*8, 120, 120, 0.3f,0.8f,1.0f, 0.25f);
}

/* O004 — order test: opaque over translucent vs translucent over opaque */
static void o004(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O004");
    /* left: translucent green over opaque red */
    rect(cv, ox+20, oy+40, 80, 150, 1.0f,0.2f,0.2f, 1.0f);
    rect(cv, ox+45, oy+70, 80, 150, 0.2f,1.0f,0.2f, 0.5f);
    /* right: opaque green over translucent red */
    rect(cv, ox+130, oy+40, 80, 150, 1.0f,0.2f,0.2f, 0.5f);
    rect(cv, ox+150, oy+70, 60, 120, 0.2f,1.0f,0.2f, 1.0f);
}

/* O005 — translucent white veil over busy backdrop (frost without blur) */
static void o005(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O005");
    busy_backdrop(cv, ox, oy);
    rect(cv, ox+30, oy+60, CONTENT_W-60, CONTENT_H-120, 1.0f,1.0f,1.0f, 0.45f);
}

/* O006 — many small translucent dots accumulate into a soft cloud */
static void o006(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O006");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=0;i<40;i++){
        float a = i*0.61f; float rr = 18+i*1.6f;
        circ(cv, cx+cosf(a)*rr*0.5f, cy+sinf(a)*rr*0.5f, 22, 1.0f,0.7f,0.1f, 0.10f);
    }
}

/* O007 — translucent stroke crosshatch over fill */
static void o007(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O007");
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.15f,0.2f,0.4f, 1.0f);
    CevgPaint* p = cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 10.0f);
    cevg_paint_set_color(p, 1.0f,1.0f,1.0f, 0.25f);
    for (int i=0;i<8;i++){
        cevg_canvas_draw_rect(cv, ox+20, oy+34+i*22, CONTENT_W-40, 1, p);
    }
    cevg_paint_destroy(p);
}

/* O008 — paint alpha vs color alpha equivalence on overlap */
static void o008(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O008");
    /* left uses color[3]=0.5; right uses paint alpha 0.5 — should match visually */
    rect(cv, ox+20, oy+40, 90, 150, 0.9f,0.3f,0.1f, 0.5f);
    rect(cv, ox+55, oy+70, 90, 120, 0.1f,0.6f,0.9f, 0.5f);
    CevgPaint* p1 = cevg_paint_create();
    cevg_paint_set_color(p1, 0.9f,0.3f,0.1f,1.0f); cevg_paint_set_alpha(p1,0.5f);
    cevg_canvas_draw_rect(cv, ox+130, oy+40, 90, 150, p1); cevg_paint_destroy(p1);
    CevgPaint* p2 = cevg_paint_create();
    cevg_paint_set_color(p2, 0.1f,0.6f,0.9f,1.0f); cevg_paint_set_alpha(p2,0.5f);
    cevg_canvas_draw_rect(cv, ox+165, oy+70, 60, 120, p2); cevg_paint_destroy(p2);
}

/* O009 — translucent gradient over opaque shapes */
static void o009(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O009");
    circ(cv, ox+80, oy+90, 50, 1.0f,0.2f,0.2f,1.0f);
    circ(cv, ox+150,oy+140,50, 0.2f,0.8f,0.3f,1.0f);
    uint32_t cols[]={0x800000FF, 0x80FF00FF};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox, oy, ox+CONTENT_W, oy+CONTENT_H, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+20, oy+20, CONTENT_W-40, CONTENT_H-40, g);
    cevg_paint_destroy(g);
}

/* O010 — concentric translucent rings (alpha buildup toward center) */
static void o010(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O010");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=0;i<6;i++) circ(cv, cx, cy, 100-i*15, 0.2f,0.9f,1.0f, 0.22f);
}

/* O011 — checkerboard of alternating translucent squares */
static void o011(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O011");
    for (int r=0;r<6;r++) for (int c=0;c<6;c++){
        float a = ((r+c)&1)?0.7f:0.3f;
        rect(cv, ox+16+c*32, oy+24+r*32, 30, 30, 0.9f,0.5f,0.2f, a);
    }
    rect(cv, ox+50, oy+50, 130, 130, 0.2f,0.4f,1.0f, 0.4f);
}

/* O012 — translucent triangles (path) overlapping */
static void o012(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O012");
    for (int i=0;i<3;i++){
        CevgPath* t=cevg_path_create();
        float bx=ox+40+i*40, by=oy+40+i*30;
        cevg_path_move_to(t, bx, by+120);
        cevg_path_line_to(t, bx+90, by+120);
        cevg_path_line_to(t, bx+45, by);
        cevg_path_close(t);
        CevgPaint* p=cevg_paint_create();
        float r=(i==0),g=(i==1),b=(i==2);
        cevg_paint_set_color(p, r,g,b, 0.5f);
        cevg_canvas_draw_path(cv, t, p);
        cevg_paint_destroy(p); cevg_path_destroy(t);
    }
}

/* O013 — translucent rounded rects fanned out */
static void o013(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O013");
    for (int i=0;i<5;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.1f+i*0.18f, 0.9f-i*0.15f, 0.5f, 0.45f);
        cevg_canvas_draw_round_rect(cv, ox+20+i*16, oy+40+i*10, 110, 110, 18,18, p);
        cevg_paint_destroy(p);
    }
}

/* O014 — translucent ovals crossing */
static void o014(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O014");
    CevgPaint* p1=cevg_paint_create(); cevg_paint_set_color(p1,1.0f,0.3f,0.5f,0.55f);
    cevg_canvas_draw_oval(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 95, 45, p1); cevg_paint_destroy(p1);
    CevgPaint* p2=cevg_paint_create(); cevg_paint_set_color(p2,0.3f,0.7f,1.0f,0.55f);
    cevg_canvas_draw_oval(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 45, 95, p2); cevg_paint_destroy(p2);
}

/* O015 — fully transparent fill should be a no-op (overlap unaffected) */
static void o015(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O015");
    rect(cv, ox+40, oy+50, 150, 140, 0.9f,0.7f,0.1f, 1.0f);
    rect(cv, ox+60, oy+70, 110, 100, 0.0f,0.0f,0.0f, 0.0f); /* invisible */
    rect(cv, ox+90, oy+90, 80, 80, 0.2f,0.3f,0.9f, 1.0f);
}

/* O016 — alpha gradient ramp strips overlapping a vertical bar */
static void o016(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=0; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O016");
    rect(cv, ox+CONTENT_W*0.5f-15, oy+20, 30, CONTENT_H-40, 1.0f,1.0f,1.0f,1.0f);
    for (int i=0;i<10;i++){
        float a=(i+1)/10.0f;
        rect(cv, ox+20, oy+24+i*20, CONTENT_W-40, 16, 0.9f,0.2f,0.4f, a);
    }
}

/* ================================================================== */
/* O017-O032 : GROUP (LAYER) OPACITY vs PER-SHAPE, NESTED LAYERS       */
/* ================================================================== */

/* O017 — two overlapping opaque rects inside a 50% layer: the OVERLAP does
 * NOT darken (group opacity), unlike O001 where each shape is translucent. */
static void o017(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O017");
    float b[4]={ox+10,oy+20,ox+CONTENT_W-10,oy+CONTENT_H-10};
    cevg_canvas_save_layer(cv, 0.5f, b, NULL);
    rect(cv, ox+30, oy+40, 110, 110, 1.0f,0.2f,0.2f, 1.0f);
    rect(cv, ox+90, oy+90, 110, 110, 0.2f,0.4f,1.0f, 1.0f);
    cevg_canvas_restore_layer(cv);
}

/* O018 — side by side: per-shape 50% (left) vs group 50% (right) */
static void o018(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O018");
    /* left: per-shape alpha */
    rect(cv, ox+16, oy+50, 50, 60, 1.0f,0.2f,0.2f, 0.5f);
    rect(cv, ox+40, oy+90, 50, 60, 0.2f,0.4f,1.0f, 0.5f);
    /* right: group alpha */
    float b[4]={ox+120,oy+30,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 0.5f, b, NULL);
    rect(cv, ox+130, oy+50, 50, 60, 1.0f,0.2f,0.2f, 1.0f);
    rect(cv, ox+154, oy+90, 50, 60, 0.2f,0.4f,1.0f, 1.0f);
    cevg_canvas_restore_layer(cv);
}

/* O019 — nested layers, each 0.6: effective center alpha multiplies */
static void o019(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O019");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 0.6f, b, NULL);
    rect(cv, ox+24, oy+40, 120, 150, 1.0f,0.3f,0.1f, 1.0f);
    cevg_canvas_save_layer(cv, 0.6f, b, NULL);
    rect(cv, ox+80, oy+60, 120, 130, 0.1f,0.6f,1.0f, 1.0f);
    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore_layer(cv);
}

/* O020 — layer alpha applied to a translucent stack (compound) */
static void o020(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O020");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 0.7f, b, NULL);
    for (int i=0;i<4;i++) circ(cv, ox+70+i*18, oy+90+i*12, 45, 0.9f,0.8f,0.2f, 0.5f);
    cevg_canvas_restore_layer(cv);
}

/* O021 — opaque shape, then layer-with-alpha containing overlapping shapes
 * that intersect the first (group composited over existing content) */
static void o021(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O021");
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.15f,0.2f,0.45f, 1.0f);
    float b[4]={ox+30,oy+40,ox+CONTENT_W-30,oy+CONTENT_H-40};
    cevg_canvas_save_layer(cv, 0.55f, b, NULL);
    circ(cv, ox+90, oy+110, 50, 1.0f,0.3f,0.3f,1.0f);
    circ(cv, ox+140,oy+120, 50, 0.3f,1.0f,0.3f,1.0f);
    cevg_canvas_restore_layer(cv);
}

/* O022 — three separate small layers (no overlap between layers) */
static void o022(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O022");
    float a[4]={ox+16,oy+30,ox+90,oy+120};
    cevg_canvas_save_layer(cv,0.5f,a,NULL); rect(cv,ox+16,oy+30,74,90,1,0,0,1); cevg_canvas_restore_layer(cv);
    float b[4]={ox+80,oy+60,ox+160,oy+150};
    cevg_canvas_save_layer(cv,0.7f,b,NULL); rect(cv,ox+80,oy+60,80,90,0,1,0,1); cevg_canvas_restore_layer(cv);
    float c[4]={ox+140,oy+90,ox+220,oy+200};
    cevg_canvas_save_layer(cv,0.9f,c,NULL); rect(cv,ox+140,oy+90,80,110,0,0,1,1); cevg_canvas_restore_layer(cv);
}

/* O023 — layer alpha 1.0 over a stack should equal no layer */
static void o023(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O023");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 1.0f, b, NULL);
    rect(cv, ox+30, oy+40, 110, 110, 1.0f,0.2f,0.2f, 1.0f);
    rect(cv, ox+90, oy+90, 110, 110, 0.2f,0.4f,1.0f, 1.0f);
    cevg_canvas_restore_layer(cv);
}

/* O024 — deep nesting (4 layers) with a shape at each level */
static void o024(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=1; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O024");
    float b[4]={ox+8,oy+14,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv,0.85f,b,NULL); rect(cv,ox+20,oy+30,180,40,1,0.2f,0.2f,1);
    cevg_canvas_save_layer(cv,0.85f,b,NULL); rect(cv,ox+20,oy+78,180,40,0.2f,1,0.2f,1);
    cevg_canvas_save_layer(cv,0.85f,b,NULL); rect(cv,ox+20,oy+126,180,40,0.2f,0.4f,1,1);
    cevg_canvas_save_layer(cv,0.85f,b,NULL); rect(cv,ox+20,oy+174,180,30,1,0.8f,0.1f,1);
    cevg_canvas_restore_layer(cv); cevg_canvas_restore_layer(cv);
    cevg_canvas_restore_layer(cv); cevg_canvas_restore_layer(cv);
}

/* O025-O032: a representative spread of BLEND MODES over a real backdrop.
 * Each: opaque colored backdrop rect, then a second rect with the blend mode
 * overlapping it, so the OVERLAP shows the blended result. */
static void blend_over(CevgCanvas* cv, CevgTypeface* face, int col, int row,
                       const char* lbl, CevgBlendMode mode) {
    float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,lbl);
    /* backdrop: gradient-ish two-tone so blend is visible */
    rect(cv, ox+24, oy+40, 120, 150, 0.95f,0.75f,0.15f, 1.0f); /* warm */
    rect(cv, ox+24, oy+115, 120, 75, 0.15f,0.55f,0.95f, 1.0f); /* cool lower half */
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.85f,0.1f,0.45f, 1.0f);
    cevg_paint_set_blend_mode(p, mode);
    cevg_canvas_draw_rect(cv, ox+80, oy+60, 120, 130, p);
    cevg_paint_destroy(p);
}
static void o025(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,8,1,"O025 Mul", kCevgBlendMode_Multiply); }
static void o026(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,9,1,"O026 Scr", kCevgBlendMode_Screen); }
static void o027(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,10,1,"O027 Ovl", kCevgBlendMode_Overlay); }
static void o028(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,11,1,"O028 Drk", kCevgBlendMode_Darken); }
static void o029(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,12,1,"O029 Lgt", kCevgBlendMode_Lighten); }
static void o030(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,13,1,"O030 Dif", kCevgBlendMode_Difference); }
static void o031(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,14,1,"O031 Dge", kCevgBlendMode_ColorDodge); }
static void o032(CevgCanvas* cv, CevgTypeface* face){ blend_over(cv,face,15,1,"O032 Excl", kCevgBlendMode_Exclusion); }

/* ================================================================== */
/* O033-O048 : PORTER-DUFF in an ISOLATED (transparent) LAYER          */
/* These show the *intuitive* clip-to-shape behaviour that direct      */
/* compositing onto an opaque surface cannot (dst alpha = 1 there).    */
/* Pattern: open a transparent layer, draw DST (red), then SRC (blue)  */
/* with the Porter-Duff mode, restore. The layer's own alpha makes     */
/* In/Out/ATop behave as expected.                                     */
/* ================================================================== */
static void pd_layer(CevgCanvas* cv, CevgTypeface* face, int col, int row,
                     const char* lbl, CevgBlendMode mode) {
    float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,lbl);
    float b[4]={ox+10,oy+20,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 1.0f, b, NULL);   /* transparent isolated layer */
    /* DST: red square (left) */
    rect(cv, ox+30, oy+45, 110, 110, 0.95f,0.15f,0.15f, 1.0f);
    /* SRC: blue square (right), with the blend mode */
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.15f,0.35f,0.95f, 1.0f);
    cevg_paint_set_blend_mode(p, mode);
    cevg_canvas_draw_rect(cv, ox+90, oy+85, 110, 110, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore_layer(cv);
}
static void o033(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,0,2,"O033 Src",   kCevgBlendMode_Src); }
static void o034(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,1,2,"O034 SrcIn", kCevgBlendMode_SrcIn); }
static void o035(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,2,2,"O035 SrcOut",kCevgBlendMode_SrcOut); }
static void o036(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,3,2,"O036 SrcATop",kCevgBlendMode_SrcATop); }
static void o037(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,4,2,"O037 DstIn", kCevgBlendMode_DstIn); }
static void o038(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,5,2,"O038 DstOut",kCevgBlendMode_DstOut); }
static void o039(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,6,2,"O039 DstATop",kCevgBlendMode_DstATop); }
static void o040(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,7,2,"O040 Xor",   kCevgBlendMode_Xor); }
static void o041(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,8,2,"O041 Plus",  kCevgBlendMode_Plus); }
static void o042(CevgCanvas* cv, CevgTypeface* face){ pd_layer(cv,face,9,2,"O042 DstOver",kCevgBlendMode_DstOver); }

/* O043 — Clear inside a layer punches a hole revealing layer transparency
 * (then the layer composites over a backdrop, so the hole shows backdrop). */
static void o043(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O043 Clr");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+20,oy+30,ox+CONTENT_W-20,oy+CONTENT_H-30};
    cevg_canvas_save_layer(cv, 1.0f, b, NULL);
    rect(cv, ox+30, oy+45, CONTENT_W-60, CONTENT_H-90, 0.1f,0.1f,0.12f, 1.0f);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0,0,0, 1.0f);
    cevg_paint_set_blend_mode(p, kCevgBlendMode_Clear);
    cevg_canvas_draw_circle(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 45, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore_layer(cv);
}

/* O044 — Multiply layer (group) over backdrop: whole group multiplies */
static void o044(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O044 LMul");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+20,oy+30,ox+CONTENT_W-20,oy+CONTENT_H-30};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blend_mode(lp, kCevgBlendMode_Multiply);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    rect(cv, ox+40, oy+50, 70, 120, 0.7f,0.7f,0.9f, 1.0f);
    rect(cv, ox+110,oy+70, 70, 110, 0.9f,0.7f,0.7f, 1.0f);
    cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}

/* O045 — Screen layer (group) over dark backdrop: brightens overlap region */
static void o045(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O045 LScr");
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.08f,0.08f,0.12f,1.0f);
    float b[4]={ox+20,oy+30,ox+CONTENT_W-20,oy+CONTENT_H-30};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blend_mode(lp, kCevgBlendMode_Screen);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    circ(cv, ox+90, oy+110, 45, 0.9f,0.2f,0.2f,1.0f);
    circ(cv, ox+140,oy+120, 45, 0.2f,0.2f,0.9f,1.0f);
    cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}

/* O046 — blend mode between two translucent shapes (mode + alpha combo) */
static void o046(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O046");
    rect(cv, ox+30, oy+45, 120, 120, 0.95f,0.6f,0.1f, 0.8f);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.1f,0.5f,0.95f, 0.8f);
    cevg_paint_set_blend_mode(p, kCevgBlendMode_HardLight);
    cevg_canvas_draw_rect(cv, ox+80, oy+85, 120, 120, p);
    cevg_paint_destroy(p);
}

/* O047 — many overlapping Plus (additive) circles -> blow out to white */
static void o047(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O047 Add");
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.04f,0.04f,0.06f,1.0f);
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    const float colsv[3][3]={{0.5f,0.1f,0.1f},{0.1f,0.5f,0.1f},{0.1f,0.1f,0.5f}};
    for (int i=0;i<3;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, colsv[i][0],colsv[i][1],colsv[i][2], 1.0f);
        cevg_paint_set_blend_mode(p, kCevgBlendMode_Plus);
        float a=i*2.094f;
        cevg_canvas_draw_circle(cv, cx+cosf(a)*28, cy+sinf(a)*28, 48, p);
        cevg_paint_destroy(p);
    }
}

/* O048 — Difference of overlapping gradient and solid */
static void o048(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=2; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O048 Dif");
    uint32_t cols[]={0xFFFF2020,0xFF2020FF};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox+20,oy, ox+CONTENT_W-20,oy, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+20, oy+40, CONTENT_W-40, 150, g);
    cevg_paint_destroy(g);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 1.0f,1.0f,1.0f,1.0f);
    cevg_paint_set_blend_mode(p, kCevgBlendMode_Difference);
    cevg_canvas_draw_circle(cv, ox+CONTENT_W*0.5f, oy+115, 55, p);
    cevg_paint_destroy(p);
}

/* ================================================================== */
/* O049-O064 : BACKDROP COPY / FROSTED GLASS / BACKDROP BLUR           */
/* Each draws busy content, then a backdrop-blur (or shadow) layer     */
/* confined to a sub-rect, so the panel shows a blurred copy of what's */
/* behind it — the classic "frosted glass" overlap.                    */
/* ================================================================== */

static void backdrop_panel(CevgCanvas* cv, CevgTypeface* face, int col, int row,
                           const char* lbl, float sigma, float panel_alpha) {
    float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,lbl);
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+40,oy+55,ox+CONTENT_W-40,oy+CONTENT_H-55};
    CevgPaint* lp=cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, sigma, sigma);
    cevg_canvas_save_layer(cv, panel_alpha, b, lp);
    /* a translucent tint inside the frosted panel */
    rect(cv, ox+40, oy+55, CONTENT_W-80, CONTENT_H-110, 1.0f,1.0f,1.0f, 0.18f);
    cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}
static void o049(CevgCanvas* cv, CevgTypeface* face){ backdrop_panel(cv,face,0,3,"O049 Frost4", 4.0f, 1.0f); }
static void o050(CevgCanvas* cv, CevgTypeface* face){ backdrop_panel(cv,face,1,3,"O050 Frost8", 8.0f, 1.0f); }
static void o051(CevgCanvas* cv, CevgTypeface* face){ backdrop_panel(cv,face,2,3,"O051 Frost16",16.0f, 1.0f); }
static void o052(CevgCanvas* cv, CevgTypeface* face){ backdrop_panel(cv,face,3,3,"O052 FrostA", 10.0f, 0.6f); }

/* O053 — frosted circle panel over content (non-rect bounds via clip) */
static void o053(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O053 FrCirc");
    busy_backdrop(cv, ox, oy);
    cevg_canvas_save(cv);
    CevgPath* cp = make_circle_path(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 70);
    cevg_canvas_clip_path(cv, cp);
    float b[4]={ox+CONTENT_W*0.5f-72,oy+CONTENT_H*0.5f-72,ox+CONTENT_W*0.5f+72,oy+CONTENT_H*0.5f+72};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 10.0f, 10.0f);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
    cevg_path_destroy(cp);
}

/* O054 — two stacked frosted panels (frost of a frost) */
static void o054(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O054 Frost2");
    busy_backdrop(cv, ox, oy);
    float b1[4]={ox+24,oy+40,ox+150,oy+170};
    CevgPaint* lp1=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp1, 6.0f, 6.0f);
    cevg_canvas_save_layer(cv,1.0f,b1,lp1);
    rect(cv, ox+24,oy+40, 126,130, 1,1,1, 0.12f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp1);
    float b2[4]={ox+90,oy+80,ox+216,oy+205};
    CevgPaint* lp2=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp2, 6.0f, 6.0f);
    cevg_canvas_save_layer(cv,1.0f,b2,lp2);
    rect(cv, ox+90,oy+80, 126,125, 1,1,1, 0.12f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp2);
}

/* O055 — backdrop shadow layer (inner content casts/uses backdrop) */
static void o055(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O055 BkShd");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+45,oy+55,ox+CONTENT_W-45,oy+CONTENT_H-55};
    CevgPaint* lp=cevg_paint_create();
    cevg_paint_set_backdrop_shadow(lp, 4.0f, 4.0f, 4.0f, 0xAA000000);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    rect(cv, ox+55, oy+65, CONTENT_W-110, CONTENT_H-130, 1.0f,1.0f,1.0f, 0.6f);
    cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}

/* O056 — frosted panel with sharp opaque UI elements on top of it */
static void o056(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O056 Card");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+30,oy+45,ox+CONTENT_W-30,oy+CONTENT_H-45};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 12.0f, 12.0f);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    rect(cv, ox+30,oy+45, CONTENT_W-60, CONTENT_H-90, 0.1f,0.12f,0.2f, 0.45f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    /* sharp UI on top */
    circ(cv, ox+55, oy+70, 8, 0.9f,0.3f,0.3f,1.0f);
    circ(cv, ox+80, oy+70, 8, 0.95f,0.75f,0.2f,1.0f);
    rect(cv, ox+50, oy+95, CONTENT_W-100, 6, 1,1,1,0.8f);
    rect(cv, ox+50, oy+115, CONTENT_W-130, 6, 1,1,1,0.5f);
}

/* O057-O060 : backdrop blur over a GRADIENT backdrop (smooth source) */
static void grad_backdrop(CevgCanvas* cv, float ox, float oy) {
    uint32_t cols[]={0xFFFF3060,0xFF30C0FF,0xFFFFD020};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox,oy, ox+CONTENT_W,oy+CONTENT_H, cols, NULL, 3, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox, oy, CONTENT_W, CONTENT_H, g);
    cevg_paint_destroy(g);
}
static void o057(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O057 GFrost");
    grad_backdrop(cv, ox, oy);
    float b[4]={ox+40,oy+55,ox+CONTENT_W-40,oy+CONTENT_H-55};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 10.0f, 10.0f);
    cevg_canvas_save_layer(cv, 1.0f, b, lp); cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}
static void o058(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O058 GTint");
    grad_backdrop(cv, ox, oy);
    rect(cv, ox+30, oy+50, CONTENT_W-60, CONTENT_H-100, 0.0f,0.0f,0.0f, 0.35f);
}

/* O059 — backdrop blur then opaque shapes overlapping the panel edge */
static void o059(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O059 Edge");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+60,oy+60,ox+CONTENT_W-20,oy+CONTENT_H-20};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 9.0f, 9.0f);
    cevg_canvas_save_layer(cv, 1.0f, b, lp);
    rect(cv, ox+60,oy+60, CONTENT_W-80, CONTENT_H-80, 1,1,1,0.15f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    circ(cv, ox+60, oy+60, 26, 0.95f,0.2f,0.5f,1.0f); /* straddles the panel corner */
}

/* O060 — asymmetric backdrop blur (sigma_x != sigma_y) */
static void o060(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O060 Aniso");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+30,oy+55,ox+CONTENT_W-30,oy+CONTENT_H-55};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 16.0f, 2.0f);
    cevg_canvas_save_layer(cv, 1.0f, b, lp); cevg_canvas_restore_layer(cv);
    cevg_paint_destroy(lp);
}

/* O061 — small frosted "tooltip" overlapping a big opaque shape */
static void o061(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O061 Tip");
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.85f,0.3f,0.2f,1.0f);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 60, 0.2f,0.8f,0.4f,1.0f);
    float b[4]={ox+70,oy+80,ox+170,oy+140};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 8.0f, 8.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+70,oy+80, 100,60, 1,1,1,0.25f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O062 — frosted panel under a translucent gradient (layered glass) */
static void o062(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O062 Glass");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+30,oy+50,ox+CONTENT_W-30,oy+CONTENT_H-50};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 12.0f, 12.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp); cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    uint32_t cols[]={0x40FFFFFF,0x10FFFFFF};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox, oy+50, ox, oy+CONTENT_H-50, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+30, oy+50, CONTENT_W-60, CONTENT_H-100, g);
    cevg_paint_destroy(g);
}

/* O063 — frosted strip overlapping cell edges (clip to content) */
static void o063(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O063 Strip");
    busy_backdrop(cv, ox, oy);
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, CONTENT_W, CONTENT_H);
    float b[4]={ox-10,oy+95,ox+CONTENT_W+10,oy+150};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 10.0f, 10.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox-10,oy+95, CONTENT_W+20, 55, 1,1,1,0.18f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
}

/* O064 — backdrop blur with content drawn INSIDE the layer too */
static void o064(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=3; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O064 Mix");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+35,oy+50,ox+CONTENT_W-35,oy+CONTENT_H-50};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 10.0f, 10.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+35,oy+50, CONTENT_W-70, CONTENT_H-100, 0.1f,0.1f,0.15f, 0.4f);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 30, 1.0f,0.85f,0.2f,1.0f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* ================================================================== */
/* O065-O080 : CLIP + OVERLAP (clip then stack multiple draws)         */
/* ================================================================== */

/* O065 — clip to rect, then draw several overlapping shapes that exceed it */
static void o065(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O065");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+40, oy+50, CONTENT_W-80, CONTENT_H-100);
    rect(cv, ox+10, oy+20, 120, 200, 1,0.2f,0.2f,1);
    rect(cv, ox+90, oy+60, 140, 160, 0.2f,0.4f,1,0.7f);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 90, 0.2f,1,0.4f,0.6f);
    cevg_canvas_restore(cv);
}

/* O066 — nested clips: intersection only, with overlapping fills inside */
static void o066(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O066");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+20, oy+20, 170, 170);
    cevg_canvas_clip_rect(cv, ox+60, oy+60, 160, 160);
    for (int i=0;i<5;i++) circ(cv, ox+60+i*20, oy+70+i*20, 55, (i&1)?1.0f:0.2f, 0.6f, (i&1)?0.2f:1.0f, 0.6f);
    cevg_canvas_restore(cv);
}

/* O067 — clip to star path, stacked translucent rects inside */
static void o067(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O067 Star");
    cevg_canvas_save(cv);
    CevgPath* st = make_star(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 95, 40);
    cevg_canvas_clip_path(cv, st);
    busy_backdrop(cv, ox, oy);
    cevg_canvas_restore(cv);
    cevg_path_destroy(st);
}

/* O068 — clip to circle, draw blurred content inside */
static void o068(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O068 ClBlur");
    cevg_canvas_save(cv);
    CevgPath* cp = make_circle_path(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 75);
    cevg_canvas_clip_path(cv, cp);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 6.0f,6.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+50,oy+60, 60,60, 1,0.2f,0.2f,1);
    rect(cv, ox+120,oy+110, 60,60, 0.2f,0.3f,1,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv); cevg_path_destroy(cp);
}

/* O069 — clip rect, then a layer with overlapping shapes (clip ∩ layer) */
static void o069(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O069");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+30, oy+40, CONTENT_W-60, CONTENT_H-80);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv,0.6f,b,NULL);
    rect(cv, ox+20,oy+30, 130,170, 1,0.3f,0.1f,1);
    rect(cv, ox+90,oy+70, 130,150, 0.1f,0.5f,1,1);
    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);
}

/* O070 — overlapping clip regions via save/restore sequencing */
static void o070(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O070");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+20, oy+30, 110, 170);
    rect(cv, ox+10, oy+20, 200, 200, 0.9f,0.4f,0.2f, 1.0f);
    cevg_canvas_restore(cv);
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+100, oy+60, 120, 140);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 90, 0.2f,0.5f,1.0f, 0.7f);
    cevg_canvas_restore(cv);
}

/* O071 — clip path (circle) then blend-mode rect overlap inside */
static void o071(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O071");
    cevg_canvas_save(cv);
    CevgPath* cp = make_circle_path(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 80);
    cevg_canvas_clip_path(cv, cp);
    rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-60, 0.95f,0.7f,0.1f,1.0f);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.2f,0.3f,0.9f,1.0f);
    cevg_paint_set_blend_mode(p, kCevgBlendMode_Multiply);
    cevg_canvas_draw_circle(cv, ox+CONTENT_W*0.5f+20, oy+CONTENT_H*0.5f+20, 60, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore(cv); cevg_path_destroy(cp);
}

/* O072 — transformed clip: rotate then clip rect, draw overlap */
static void o072(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O072 Rot");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, 20.0f);
    cevg_canvas_clip_rect(cv, -80, -55, 160, 110);
    rect(cv, ox-200, oy-200, 1, 1, 0,0,0,0); /* noop to keep coords sane */
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.9f,0.3f,0.4f,1.0f);
    cevg_canvas_draw_rect(cv, -100, -90, 130, 180, p);
    cevg_paint_set_color(p, 0.2f,0.6f,1.0f,0.7f);
    cevg_canvas_draw_rect(cv, -20, -60, 130, 160, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore(cv);
}

/* O073 — clip to rect, stack of translucent circles overflowing */
static void o073(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O073");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+30, oy+30, CONTENT_W-60, CONTENT_H-60);
    for (int i=0;i<8;i++){
        float a=i*0.78f;
        circ(cv, ox+CONTENT_W*0.5f+cosf(a)*60, oy+CONTENT_H*0.5f+sinf(a)*60, 50,
             0.5f+0.5f*sinf(a), 0.4f, 0.5f+0.5f*cosf(a), 0.4f);
    }
    cevg_canvas_restore(cv);
}

/* O074 — nested layer inside a clip inside another layer */
static void o074(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O074");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    cevg_canvas_save_layer(cv, 0.85f, b, NULL);
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+40, oy+50, CONTENT_W-80, CONTENT_H-100);
    cevg_canvas_save_layer(cv, 0.7f, b, NULL);
    rect(cv, ox+20,oy+30, 130,170, 1,0.3f,0.2f,1);
    rect(cv, ox+90,oy+70, 130,150, 0.2f,0.4f,1,1);
    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);
    cevg_canvas_restore_layer(cv);
}

/* O075 — clip-path heart-ish (two circles + triangle via path) with overlap */
static void o075(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O075");
    cevg_canvas_save(cv);
    CevgPath* p = cevg_path_create();
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    cevg_path_move_to(p, cx, cy+70);
    cevg_path_cubic_to(p, cx-90, cy-10, cx-30, cy-80, cx, cy-30);
    cevg_path_cubic_to(p, cx+30, cy-80, cx+90, cy-10, cx, cy+70);
    cevg_path_close(p);
    cevg_canvas_clip_path(cv, p);
    busy_backdrop(cv, ox, oy);
    cevg_canvas_restore(cv); cevg_path_destroy(p);
}

/* O076 — clip then draw a frosted panel (clip ∩ backdrop blur) */
static void o076(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O076");
    busy_backdrop(cv, ox, oy);
    cevg_canvas_save(cv);
    CevgPath* st = make_star(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 92, 42);
    cevg_canvas_clip_path(cv, st);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 12.0f, 12.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp); cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv); cevg_path_destroy(st);
}

/* O077 — multiple clip rects (grid) each with a different overlap */
static void o077(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O077");
    for (int q=0;q<4;q++){
        float qx=ox+16+(q%2)*108, qy=oy+24+(q/2)*98;
        cevg_canvas_save(cv);
        cevg_canvas_clip_rect(cv, qx, qy, 96, 86);
        circ(cv, qx+48, qy+43, 60, (q&1)?1.0f:0.2f, 0.6f, (q&2)?1.0f:0.2f, 1.0f);
        circ(cv, qx+20, qy+20, 50, 0.95f,0.85f,0.1f, 0.5f);
        cevg_canvas_restore(cv);
    }
}

/* O078 — clip + color matrix layer (grayscale a clipped region) */
static void o078(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O078 Gray");
    busy_backdrop(cv, ox, oy);
    float mat[20]={0.299f,0.587f,0.114f,0,0, 0.299f,0.587f,0.114f,0,0,
                   0.299f,0.587f,0.114f,0,0, 0,0,0,1,0};
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+CONTENT_W*0.5f, oy+20, CONTENT_W*0.5f-20, CONTENT_H-40);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    busy_backdrop(cv, ox, oy);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
}

/* O079 — clip animation-style: concentric clip rings drawn as overlap */
static void o079(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O079");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=0;i<4;i++){
        cevg_canvas_save(cv);
        CevgPath* cp=make_circle_path(cx,cy, 90-i*20);
        cevg_canvas_clip_path(cv, cp);
        rect(cv, ox+20, oy+20+i*45, CONTENT_W-40, 45, (i&1)?0.9f:0.2f, 0.5f, (i&1)?0.2f:0.9f, 1.0f);
        cevg_canvas_restore(cv); cevg_path_destroy(cp);
    }
}

/* O080 — clip to thin strip, draw rotated overlapping bars (moire-ish) */
static void o080(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=4; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O080");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+24, oy+24, CONTENT_W-48, CONTENT_H-48);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p, 0.95f,0.3f,0.4f, 0.5f);
    for (int i=0;i<10;i++) cevg_canvas_draw_rect(cv, ox+10+i*22, oy+10, 10, CONTENT_H-20, p);
    cevg_paint_set_color(p, 0.2f,0.6f,1.0f, 0.5f);
    for (int i=0;i<10;i++) cevg_canvas_draw_rect(cv, ox+10, oy+10+i*22, CONTENT_W-20, 10, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore(cv);
}

/* ================================================================== */
/* O081-O096 : FILTER LAYERS over overlapping content (blur/shadow/mat)*/
/* ================================================================== */

/* O081 — blur layer over two overlapping shapes (whole group blurs together) */
static void o081(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O081 Blur");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 6.0f,6.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+40,oy+50, 90,90, 1,0.2f,0.2f,1);
    rect(cv, ox+100,oy+100, 90,90, 0.2f,0.3f,1,0.8f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O082 — drop shadow on a group of overlapping shapes (single silhouette) */
static void o082(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O082 Shdw");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp, 5,5,4, 0xFF000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    circ(cv, ox+90, oy+90, 40, 1,0.85f,0.1f,1);
    circ(cv, ox+130, oy+120, 40, 1,0.5f,0.1f,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O083 — overlapping shadowed cards (each its own shadow layer) */
static void o083(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O083 Cards");
    for (int i=0;i<3;i++){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp, 4,4,3, 0xCC000000);
        cevg_canvas_save_layer(cv,1.0f,b,lp);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.9f-i*0.2f, 0.6f, 0.3f+i*0.2f, 1.0f);
        cevg_canvas_draw_round_rect(cv, ox+30+i*30, oy+40+i*30, 100, 90, 10,10, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O084 — blur layer + then a sharp overlay (focus-on-blur) */
static void o084(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O084 Focus");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 8.0f,8.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    busy_backdrop(cv, ox, oy);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 36, 1,1,1,0.95f);
}

/* O085 — color-matrix (invert) layer over overlapping shapes */
static void o085(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O085 Inv");
    float mat[20]={-1,0,0,0,1, 0,-1,0,0,1, 0,0,-1,0,1, 0,0,0,1,0};
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+40,oy+50, 100,100, 0.9f,0.2f,0.2f,1);
    rect(cv, ox+100,oy+90, 100,100, 0.2f,0.7f,0.3f,0.8f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O086 — blur + drop shadow combined on one layer */
static void o086(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O086 B+S");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create();
    cevg_paint_set_blur(lp, 3.0f,3.0f);
    cevg_paint_set_drop_shadow(lp, 4,4,3, 0xFF000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+55,oy+55, 110,110, 0.2f,0.9f,0.6f,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O087 — overlapping translucent shapes THEN blur (alpha+blur interaction) */
static void o087(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O087");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 5.0f,5.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    for (int i=0;i<3;i++) circ(cv, ox+80+i*25, oy+100+i*15, 45, 0.9f,0.4f,0.8f, 0.5f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O088 — gradient-filled shapes overlapping inside a blur layer */
static void o088(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O088");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 4.0f,4.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    uint32_t c1[]={0xFFFF4040,0xFFFFD040};
    CevgPaint* g1=cevg_paint_create();
    cevg_paint_set_linear_gradient(g1, ox+30,oy+40, ox+150,oy+40, c1, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_circle(cv, ox+90, oy+100, 55, g1); cevg_paint_destroy(g1);
    uint32_t c2[]={0xFF40A0FF,0xFF40FFC0};
    CevgPaint* g2=cevg_paint_create();
    cevg_paint_set_radial_gradient(g2, ox+150,oy+120, 60, c2, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_circle(cv, ox+150, oy+120, 55, g2); cevg_paint_destroy(g2);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O089 — drop shadow under a clip (shadow shaped by clipped content) */
static void o089(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O089");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp, 5,5,4, 0xFF202020);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    cevg_canvas_save(cv);
    CevgPath* st=make_star(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f-6, 75, 32);
    cevg_canvas_clip_path(cv, st);
    rect(cv, ox+20,oy+20, CONTENT_W-40, CONTENT_H-40, 0.95f,0.8f,0.2f,1);
    cevg_canvas_restore(cv); cevg_path_destroy(st);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O090 — stacked blur layers of increasing sigma (depth blur) */
static void o090(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O090");
    float sig[3]={2,5,9};
    for (int i=0;i<3;i++){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, sig[i],sig[i]);
        cevg_canvas_save_layer(cv,1.0f,b,lp);
        rect(cv, ox+30+i*45, oy+60+i*30, 70, 70, (i==0),(i==1),(i==2), 1.0f);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O091 — color matrix sepia over overlapping photo-ish gradient */
static void o091(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O091 Sepia");
    float mat[20]={0.393f,0.769f,0.189f,0,0, 0.349f,0.686f,0.168f,0,0,
                   0.272f,0.534f,0.131f,0,0, 0,0,0,1,0};
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    grad_backdrop(cv, ox, oy);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 50, 1,1,1,0.5f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O092 — blur layer whose content overflows bounds (clip-by-bounds check) */
static void o092(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O092");
    float b[4]={ox+60,oy+60,ox+CONTENT_W-20,oy+CONTENT_H-20};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 5.0f,5.0f);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+10,oy+10, CONTENT_W-20, CONTENT_H-20, 0.2f,0.8f,1.0f,1.0f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O093 — group opacity + blur (translucent blurred group over backdrop) */
static void o093(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O093");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+20,oy+30,ox+CONTENT_W-20,oy+CONTENT_H-30};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 4.0f,4.0f);
    cevg_canvas_save_layer(cv, 0.6f, b, lp);
    rect(cv, ox+40,oy+50, 80,120, 0,0,0,1);
    rect(cv, ox+110,oy+70, 80,110, 1,1,1,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O094 — drop shadow then overlapping translucent highlight */
static void o094(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O094");
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp, 6,6,5, 0xFF000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* cardp=cevg_paint_create(); cevg_paint_set_color(cardp,0.2f,0.5f,0.95f,1.0f);
    cevg_canvas_draw_round_rect(cv, ox+40, oy+50, CONTENT_W-90, CONTENT_H-110, 16,16, cardp);
    cevg_paint_destroy(cardp);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    /* glossy highlight overlapping the top */
    CevgPaint* h=cevg_paint_create(); cevg_paint_set_color(h, 1,1,1, 0.3f);
    cevg_canvas_draw_round_rect(cv, ox+48, oy+58, CONTENT_W-106, 40, 14,14, h);
    cevg_paint_destroy(h);
}

/* O095 — two blur layers overlapping each other */
static void o095(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O095");
    float b1[4]={ox+16,oy+30,ox+150,oy+170};
    CevgPaint* l1=cevg_paint_create(); cevg_paint_set_blur(l1,5,5);
    cevg_canvas_save_layer(cv,1.0f,b1,l1); rect(cv,ox+30,oy+45,100,110, 1,0.3f,0.3f,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(l1);
    float b2[4]={ox+90,oy+70,ox+220,oy+205};
    CevgPaint* l2=cevg_paint_create(); cevg_paint_set_blur(l2,5,5);
    cevg_canvas_save_layer(cv,0.8f,b2,l2); rect(cv,ox+105,oy+85,100,110, 0.3f,0.4f,1,1);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(l2);
}

/* O096 — color matrix saturation boost over overlapping muted shapes */
static void o096(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=5; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O096 Sat");
    float s=1.8f, lr=0.213f, lg=0.715f, lb=0.072f;
    float mat[20]={
        lr*(1-s)+s, lg*(1-s),   lb*(1-s),   0,0,
        lr*(1-s),   lg*(1-s)+s, lb*(1-s),   0,0,
        lr*(1-s),   lg*(1-s),   lb*(1-s)+s, 0,0,
        0,0,0,1,0};
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_color_matrix(lp, mat);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+40,oy+50, 100,100, 0.5f,0.35f,0.35f,1);
    rect(cv, ox+100,oy+90, 100,100, 0.35f,0.5f,0.4f,0.8f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* ================================================================== */
/* O097-O112 : TRANSFORM + OVERLAP, rotated/scaled stacks             */
/* ================================================================== */

/* O097 — rotated overlapping rects (fan) at 50% */
static void o097(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O097 Fan");
    for (int i=0;i<6;i++){
        cevg_canvas_save(cv);
        cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
        cevg_canvas_rotate(cv, i*30.0f);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.5f+0.5f*sinf(i), 0.6f, 0.5f+0.5f*cosf(i), 0.45f);
        cevg_canvas_draw_rect(cv, -12, -90, 24, 180, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore(cv);
    }
}

/* O098 — scaled nested squares (zoom stack) translucent */
static void o098(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O098");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    for (int i=0;i<6;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.9f,0.5f,0.1f, 0.4f);
        float s=100.0f - i*16.0f;
        cevg_canvas_draw_rect(cv, -s*0.5f, -s*0.5f, s, s, p);
        cevg_paint_destroy(p);
        cevg_canvas_rotate(cv, 12.0f);
    }
    cevg_canvas_restore(cv);
}

/* O099 — rotated layer (group) with overlap composited at angle */
static void o099(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O099");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, 18.0f);
    float b[4]={-100,-100,100,100};
    cevg_canvas_save_layer(cv, 0.6f, b, NULL);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p,1,0.2f,0.2f,1); cevg_canvas_draw_rect(cv,-80,-60,110,120,p);
    cevg_paint_set_color(p,0.2f,0.4f,1,1); cevg_canvas_draw_rect(cv,-20,-30,110,110,p);
    cevg_paint_destroy(p);
    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);
}

/* O100 — overlapping shapes each with its own rotation + translucency */
static void o100(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O100");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    const float c3[3][3]={{1,0.3f,0.3f},{0.3f,1,0.4f},{0.3f,0.5f,1}};
    for (int i=0;i<3;i++){
        cevg_canvas_save(cv);
        cevg_canvas_translate(cv, cx, cy);
        cevg_canvas_rotate(cv, i*40.0f);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, c3[i][0],c3[i][1],c3[i][2], 0.55f);
        cevg_canvas_draw_round_rect(cv, -70, -28, 140, 56, 14,14, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore(cv);
    }
}

/* O101 — scale-up translucent circle stack (bullseye glow) */
static void o101(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O101");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=6;i>=1;i--) circ(cv, cx, cy, i*16.0f, 1.0f, 0.4f+0.1f*i, 0.1f, 0.25f);
}

/* O102 — transform + clip + overlap combined */
static void o102(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O102");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, 25.0f);
    cevg_canvas_clip_rect(cv, -85, -60, 170, 120);
    cevg_canvas_rotate(cv, -25.0f);
    for (int i=0;i<5;i++) {
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, (i&1)?0.95f:0.2f, 0.6f, (i&1)?0.2f:0.95f, 0.7f);
        cevg_canvas_draw_rect(cv, -90+i*36, -90, 30, 180, p);
        cevg_paint_destroy(p);
    }
    cevg_canvas_restore(cv);
}

/* O103 — mirrored (negative scale) overlap */
static void o103(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O103 Mir");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    CevgPath* tri=cevg_path_create();
    cevg_path_move_to(tri, 0,-70); cevg_path_line_to(tri, 60,40); cevg_path_line_to(tri,-60,40); cevg_path_close(tri);
    cevg_canvas_save(cv); cevg_canvas_translate(cv,cx,cy);
    CevgPaint* p=cevg_paint_create(); cevg_paint_set_color(p,0.95f,0.5f,0.1f,0.6f);
    cevg_canvas_draw_path(cv, tri, p);
    cevg_canvas_scale(cv, 1.0f, -1.0f);
    cevg_paint_set_color(p,0.1f,0.6f,0.95f,0.6f);
    cevg_canvas_draw_path(cv, tri, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore(cv); cevg_path_destroy(tri);
}

/* O104 — rotated frosted panel over busy backdrop */
static void o104(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O104");
    busy_backdrop(cv, ox, oy);
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, 15.0f);
    float b[4]={-90,-50,90,50};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp, 9,9);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* tint=cevg_paint_create(); cevg_paint_set_color(tint,1,1,1,0.15f);
    cevg_canvas_draw_rect(cv, -90,-50, 180,100, tint);
    cevg_paint_destroy(tint);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
}

/* O105 — overlapping strokes of varying width and alpha */
static void o105(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O105");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_style(p, kCevgStyle_Stroke);
    for (int i=0;i<5;i++){
        cevg_paint_set_stroke_width(p, 30.0f - i*5.0f);
        cevg_paint_set_color(p, (i&1)?1.0f:0.2f, 0.6f, (i&1)?0.2f:1.0f, 0.5f);
        cevg_canvas_draw_circle(cv, cx, cy, 40+i*10, p);
    }
    cevg_paint_destroy(p);
}

/* O106 — translation grid of overlapping translucent squares */
static void o106(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O106");
    for (int r=0;r<4;r++) for (int c=0;c<4;c++){
        rect(cv, ox+20+c*45, oy+30+r*45, 60, 60,
             0.2f+c*0.2f, 0.3f+r*0.15f, 0.9f-c*0.1f, 0.4f);
    }
}

/* O107 — pinwheel of translucent triangles around center */
static void o107(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O107");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=0;i<8;i++){
        cevg_canvas_save(cv); cevg_canvas_translate(cv,cx,cy); cevg_canvas_rotate(cv,i*45.0f);
        CevgPath* t=cevg_path_create();
        cevg_path_move_to(t,0,0); cevg_path_line_to(t,90,-22); cevg_path_line_to(t,90,22); cevg_path_close(t);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.5f+0.5f*sinf(i), 0.4f, 0.5f+0.5f*cosf(i), 0.55f);
        cevg_canvas_draw_path(cv,t,p); cevg_paint_destroy(p); cevg_path_destroy(t);
        cevg_canvas_restore(cv);
    }
}

/* O108 — overlapping scaled gradient rects */
static void o108(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O108");
    uint32_t a[]={0x99FF4040,0x9940A0FF};
    for (int i=0;i<3;i++){
        CevgPaint* g=cevg_paint_create();
        cevg_paint_set_linear_gradient(g, ox+20,oy+40+i*10, ox+CONTENT_W-20,oy+40+i*10, a, NULL, 2, kCevgTile_Clamp);
        cevg_canvas_draw_round_rect(cv, ox+20+i*14, oy+40+i*30, CONTENT_W-40-i*28, 70, 14,14, g);
        cevg_paint_destroy(g);
    }
}

/* O109 — rotated text-less label bars overlapping (UI mock) */
static void o109(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O109");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, -10.0f);
    for (int i=0;i<5;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.9f,0.9f,0.95f, 0.2f+i*0.15f);
        cevg_canvas_draw_round_rect(cv, -90, -70+i*30, 180, 20, 10,10, p);
        cevg_paint_destroy(p);
    }
    cevg_canvas_restore(cv);
}

/* O110 — clip to rounded rect, overlapping gradient + shapes (button) */
static void o110(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O110");
    cevg_canvas_save(cv);
    CevgPath* rr = cevg_path_create();
    /* rounded-rect-ish via path of a circle clip is simpler: use clip_rect + circles */
    cevg_canvas_clip_rect(cv, ox+25, oy+70, CONTENT_W-50, 90);
    uint32_t cols[]={0xFF3060FF,0xFF60D0FF};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox, oy+70, ox, oy+160, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+25, oy+70, CONTENT_W-50, 90, g); cevg_paint_destroy(g);
    CevgPaint* hl=cevg_paint_create(); cevg_paint_set_color(hl,1,1,1,0.25f);
    cevg_canvas_draw_rect(cv, ox+25, oy+70, CONTENT_W-50, 36, hl); cevg_paint_destroy(hl);
    cevg_canvas_restore(cv); cevg_path_destroy(rr);
}

/* O111 — overlapping shadowed circles (depth dots) */
static void o111(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O111");
    const float c4[4][3]={{1,0.3f,0.3f},{0.3f,1,0.4f},{0.3f,0.5f,1},{1,0.85f,0.2f}};
    for (int i=0;i<4;i++){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp,3,3,3,0xAA000000);
        cevg_canvas_save_layer(cv,1.0f,b,lp);
        circ(cv, ox+60+i*30, oy+80+i*25, 34, c4[i][0],c4[i][1],c4[i][2],1.0f);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O112 — everything: transform + clip + layer-alpha + overlap */
static void o112(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=6; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O112");
    cevg_canvas_save(cv);
    cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    cevg_canvas_rotate(cv, 12.0f);
    cevg_canvas_clip_rect(cv, -95, -80, 190, 160);
    float b[4]={-100,-100,100,100};
    cevg_canvas_save_layer(cv, 0.7f, b, NULL);
    for (int i=0;i<4;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, (i&1)?0.95f:0.2f, 0.5f, (i&2)?0.95f:0.3f, 1.0f);
        cevg_canvas_draw_circle(cv, -40+i*28, -30+i*20, 45, p);
        cevg_paint_destroy(p);
    }
    cevg_canvas_restore_layer(cv);
    cevg_canvas_restore(cv);
}

/* ================================================================== */
/* O113-O144 : COMBINED "REAL WORLD" COMPOSITIONS                      */
/* Each cell layers several techniques: backdrop, blur, clip, blend,   */
/* alpha, shadows, gradients — the kind of stacking real UIs do.       */
/* ================================================================== */

/* O113 — glass card: busy bg, frosted panel, shadowed inner chip, dots */
static void o113(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O113");
    busy_backdrop(cv, ox, oy);
    float b[4]={ox+28,oy+45,ox+CONTENT_W-28,oy+CONTENT_H-45};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp,12,12);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+28,oy+45, CONTENT_W-56, CONTENT_H-90, 0.1f,0.12f,0.2f, 0.4f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    circ(cv, ox+52, oy+70, 9, 0.95f,0.3f,0.4f,1);
    circ(cv, ox+78, oy+70, 9, 0.3f,0.85f,0.5f,1);
    rect(cv, ox+45, oy+95, CONTENT_W-90, 7, 1,1,1,0.85f);
    rect(cv, ox+45, oy+115, CONTENT_W-120, 7, 1,1,1,0.5f);
}

/* O114 — notification stack: overlapping shadowed translucent cards */
static void o114(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O114");
    for (int i=2;i>=0;i--){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp,3,4,4,0xAA000000);
        cevg_canvas_save_layer(cv, 1.0f-i*0.18f, b, lp);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.15f+i*0.1f, 0.2f+i*0.12f, 0.35f+i*0.12f, 1.0f);
        cevg_canvas_draw_round_rect(cv, ox+25+i*8, oy+45+i*38, CONTENT_W-50-i*16, 64, 12,12, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O115 — photo + grayscale-clipped half (before/after) */
static void o115(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O115");
    grad_backdrop(cv, ox, oy);
    float mat[20]={0.299f,0.587f,0.114f,0,0, 0.299f,0.587f,0.114f,0,0,
                   0.299f,0.587f,0.114f,0,0, 0,0,0,1,0};
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+CONTENT_W*0.5f, oy, CONTENT_W*0.5f, CONTENT_H);
    float b[4]={ox+10,oy+10,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_color_matrix(lp,mat);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    grad_backdrop(cv, ox, oy);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
    rect(cv, ox+CONTENT_W*0.5f-1, oy+10, 2, CONTENT_H-20, 1,1,1,0.8f);
}

/* O116 — modal: dimmed backdrop + centered frosted dialog + buttons */
static void o116(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O116");
    busy_backdrop(cv, ox, oy);
    rect(cv, ox, oy, CONTENT_W, CONTENT_H, 0,0,0,0.45f);            /* scrim */
    float b[4]={ox+34,oy+60,ox+CONTENT_W-34,oy+CONTENT_H-60};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp,8,8);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+34,oy+60, CONTENT_W-68, CONTENT_H-120, 0.15f,0.16f,0.22f,0.7f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    rect(cv, ox+50, oy+CONTENT_H-92, 60, 22, 0.2f,0.6f,1.0f,1.0f);  /* button */
    rect(cv, ox+125,oy+CONTENT_H-92, 60, 22, 0.3f,0.3f,0.4f,1.0f);
}

/* O117 — gauge: stacked translucent arcs (rings) via clipped circles */
static void o117(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O117");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    const float cc[3][3]={{1,0.3f,0.3f},{1,0.8f,0.2f},{0.2f,0.9f,0.5f}};
    for (int i=0;i<3;i++){
        cevg_canvas_save(cv);
        CevgPath* outer=make_circle_path(cx,cy,90-i*24);
        cevg_canvas_clip_path(cv, outer);
        CevgPath* inner=make_circle_path(cx,cy,76-i*24);
        /* draw ring color, then punch center by drawing bg circle */
        circ(cv, cx, cy, 90-i*24, cc[i][0],cc[i][1],cc[i][2], 0.85f);
        CevgPaint* hole=cevg_paint_create(); cevg_paint_set_color(hole, BG_R,BG_G,BG_B,1.0f);
        cevg_paint_set_blend_mode(hole, kCevgBlendMode_Src);
        cevg_canvas_draw_circle(cv, cx, cy, 76-i*24, hole); cevg_paint_destroy(hole);
        cevg_canvas_restore(cv); cevg_path_destroy(outer); cevg_path_destroy(inner);
    }
}

/* O118 — neon: blurred glow layer under sharp stroke shapes */
static void o118(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O118 Neon");
    rect(cv, ox+16, oy+24, CONTENT_W-32, CONTENT_H-48, 0.03f,0.03f,0.06f,1.0f);
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp, 7,7);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* gp=cevg_paint_create(); cevg_paint_set_style(gp,kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(gp, 6); cevg_paint_set_color(gp, 0.2f,1.0f,0.9f,1.0f);
    cevg_canvas_draw_circle(cv, cx, cy, 55, gp); cevg_paint_destroy(gp);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    CevgPaint* sp=cevg_paint_create(); cevg_paint_set_style(sp,kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(sp, 2); cevg_paint_set_color(sp, 0.8f,1.0f,1.0f,1.0f);
    cevg_canvas_draw_circle(cv, cx, cy, 55, sp); cevg_paint_destroy(sp);
}

/* O119 — progress bars overlapping with translucent fills */
static void o119(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O119");
    for (int i=0;i<4;i++){
        float y=oy+40+i*42;
        rect(cv, ox+24, y, CONTENT_W-48, 22, 1,1,1,0.12f);      /* track */
        rect(cv, ox+24, y, (CONTENT_W-48)*(0.3f+i*0.2f), 22, 0.2f+i*0.2f,0.8f-i*0.1f,1.0f-i*0.2f, 0.9f);
    }
}

/* O120 — tag chips overlapping with shadows */
static void o120(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O120");
    const float c5[5][3]={{0.9f,0.3f,0.4f},{0.3f,0.7f,0.95f},{0.95f,0.75f,0.2f},{0.5f,0.85f,0.4f},{0.7f,0.4f,0.9f}};
    for (int i=0;i<5;i++){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp,2,2,2,0x99000000);
        cevg_canvas_save_layer(cv,1.0f,b,lp);
        CevgPaint* p=cevg_paint_create(); cevg_paint_set_color(p, c5[i][0],c5[i][1],c5[i][2], 0.92f);
        cevg_canvas_draw_round_rect(cv, ox+24+(i%3)*62, oy+50+(i/3)*55, 56, 30, 15,15, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O121 — overlapping translucent gradient blobs (mesh-ish) */
static void o121(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O121");
    rect(cv, ox+16,oy+24, CONTENT_W-32, CONTENT_H-48, 0.05f,0.05f,0.1f,1.0f);
    uint32_t blobs[4][2]={{0xCCFF4060,0x00FF4060},{0xCC40C0FF,0x0040C0FF},
                          {0xCCFFD040,0x00FFD040},{0xCC60FFA0,0x0060FFA0}};
    float pos[4][2]={{70,80},{160,90},{90,160},{170,170}};
    for (int i=0;i<4;i++){
        uint32_t cc[2]={blobs[i][0],blobs[i][1]};
        CevgPaint* g=cevg_paint_create();
        cevg_paint_set_radial_gradient(g, ox+pos[i][0], oy+pos[i][1], 70, cc, NULL, 2, kCevgTile_Clamp);
        cevg_paint_set_blend_mode(g, kCevgBlendMode_Plus);
        cevg_canvas_draw_circle(cv, ox+pos[i][0], oy+pos[i][1], 70, g);
        cevg_paint_destroy(g);
    }
}

/* O122 — clipped gradient + overlapping blurred highlight (sheen) */
static void o122(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O122");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox+24, oy+40, CONTENT_W-48, CONTENT_H-80);
    uint32_t cols[]={0xFF202840,0xFF405880};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox, oy+40, ox, oy+CONTENT_H-40, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+24, oy+40, CONTENT_W-48, CONTENT_H-80, g); cevg_paint_destroy(g);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_blur(lp,8,8);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* hp=cevg_paint_create(); cevg_paint_set_color(hp,1,1,1,0.5f);
    cevg_canvas_draw_oval(cv, ox+CONTENT_W*0.5f, oy+70, 80, 22, hp); cevg_paint_destroy(hp);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
}

/* O123 — overlapping difference shapes (psychedelic) */
static void o123(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O123");
    grad_backdrop(cv, ox, oy);
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    for (int i=0;i<4;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 1,1,1, 1.0f);
        cevg_paint_set_blend_mode(p, kCevgBlendMode_Difference);
        float a=i*1.57f;
        cevg_canvas_draw_circle(cv, cx+cosf(a)*30, cy+sinf(a)*30, 50, p);
        cevg_paint_destroy(p);
    }
}

/* O124 — layered shadows for elevation steps */
static void o124(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O124");
    float sig[3]={2,5,9}; float off[3]={2,5,9};
    for (int i=0;i<3;i++){
        float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
        CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp, off[i],off[i],sig[i],0x88000000);
        cevg_canvas_save_layer(cv,1.0f,b,lp);
        CevgPaint* p=cevg_paint_create(); cevg_paint_set_color(p,0.95f,0.95f,1.0f,1.0f);
        cevg_canvas_draw_round_rect(cv, ox+30+i*55, oy+70, 50, 80, 10,10, p);
        cevg_paint_destroy(p);
        cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    }
}

/* O125 — overlapping clipped images-of-color (avatar stack) */
static void o125(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O125");
    const float c4[4][3]={{0.95f,0.4f,0.4f},{0.4f,0.7f,0.95f},{0.5f,0.85f,0.4f},{0.95f,0.8f,0.3f}};
    for (int i=0;i<4;i++){
        float cx=ox+50+i*38, cy=oy+CONTENT_H*0.5f;
        circ(cv, cx, cy, 34, BG_R,BG_G,BG_B, 1.0f);          /* ring gap */
        cevg_canvas_save(cv);
        CevgPath* cp=make_circle_path(cx,cy,30);
        cevg_canvas_clip_path(cv,cp);
        rect(cv, cx-30, cy-30, 60, 60, c4[i][0],c4[i][1],c4[i][2],1.0f);
        rect(cv, cx-30, cy-30, 60, 30, 1,1,1,0.2f);
        cevg_canvas_restore(cv); cevg_path_destroy(cp);
    }
}

/* O126 — frosted toolbar over scrolling content */
static void o126(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O126");
    for (int i=0;i<8;i++) rect(cv, ox+20, oy+30+i*22, CONTENT_W-40, 14,
                                0.3f+0.08f*i, 0.5f, 0.9f-0.06f*i, 1.0f);
    float b[4]={ox,oy+CONTENT_H-58,ox+CONTENT_W,oy+CONTENT_H};
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, CONTENT_W, CONTENT_H);
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp,10,10);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox, oy+CONTENT_H-58, CONTENT_W, 58, 0.1f,0.1f,0.15f,0.5f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
}

/* O127 — blend mode sandwich: multiply then screen on same backdrop */
static void o127(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O127");
    grad_backdrop(cv, ox, oy);
    CevgPaint* m=cevg_paint_create(); cevg_paint_set_color(m,0.3f,0.3f,0.7f,1);
    cevg_paint_set_blend_mode(m, kCevgBlendMode_Multiply);
    cevg_canvas_draw_circle(cv, ox+90, oy+100, 55, m); cevg_paint_destroy(m);
    CevgPaint* s=cevg_paint_create(); cevg_paint_set_color(s,0.8f,0.6f,0.2f,1);
    cevg_paint_set_blend_mode(s, kCevgBlendMode_Screen);
    cevg_canvas_draw_circle(cv, ox+150, oy+130, 55, s); cevg_paint_destroy(s);
}

/* O128 — group alpha over blend-mode children */
static void o128(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=7; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O128");
    grad_backdrop(cv, ox, oy);
    float b[4]={ox+20,oy+30,ox+CONTENT_W-20,oy+CONTENT_H-30};
    cevg_canvas_save_layer(cv, 0.6f, b, NULL);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p,1,1,1,1); cevg_paint_set_blend_mode(p,kCevgBlendMode_Difference);
    cevg_canvas_draw_circle(cv, ox+95, oy+110, 50, p);
    cevg_paint_set_color(p,1,0.9f,0.2f,1); cevg_paint_set_blend_mode(p,kCevgBlendMode_Screen);
    cevg_canvas_draw_circle(cv, ox+150, oy+125, 50, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore_layer(cv);
}

/* O129-O144: stress + edge cases of overlap */

/* O129 — 20 translucent layers (deep alpha accumulation perf/clip) */
static void o129(CevgCanvas* cv, CevgTypeface* face) {
    int col=0,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O129");
    for (int i=0;i<20;i++)
        rect(cv, ox+20+i*4, oy+30+i*4, 120, 120, 0.4f,0.9f,0.5f, 0.12f);
}

/* O130 — overlapping shapes straddling cell boundary (clip to content) */
static void o130(CevgCanvas* cv, CevgTypeface* face) {
    int col=1,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O130");
    cevg_canvas_save(cv);
    cevg_canvas_clip_rect(cv, ox, oy, CONTENT_W, CONTENT_H);
    circ(cv, ox, oy, 70, 0.95f,0.3f,0.3f,0.8f);
    circ(cv, ox+CONTENT_W, oy, 70, 0.3f,0.9f,0.4f,0.8f);
    circ(cv, ox, oy+CONTENT_H, 70, 0.3f,0.4f,0.95f,0.8f);
    circ(cv, ox+CONTENT_W, oy+CONTENT_H, 70, 0.95f,0.85f,0.2f,0.8f);
    circ(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 60, 1,1,1,0.5f);
    cevg_canvas_restore(cv);
}

/* O131 — nested clip + layer + blend + blur all at once */
static void o131(CevgCanvas* cv, CevgTypeface* face) {
    int col=2,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O131");
    busy_backdrop(cv, ox, oy);
    cevg_canvas_save(cv);
    CevgPath* cp=make_circle_path(ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 80);
    cevg_canvas_clip_path(cv,cp);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp,8,8);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* p=cevg_paint_create();
    cevg_paint_set_color(p,1,0.3f,0.5f,0.7f); cevg_paint_set_blend_mode(p,kCevgBlendMode_Overlay);
    cevg_canvas_draw_rect(cv, ox+40,oy+40, CONTENT_W-80, CONTENT_H-80, p);
    cevg_paint_destroy(p);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv); cevg_path_destroy(cp);
}

/* O132 — alpha-fade gradient over overlapping opaque shapes */
static void o132(CevgCanvas* cv, CevgTypeface* face) {
    int col=3,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O132");
    rect(cv, ox+30,oy+40, 100,140, 1,0.3f,0.2f,1);
    rect(cv, ox+90,oy+60, 100,130, 0.2f,0.5f,1,1);
    uint32_t cols[]={0x00000000,0xCC000000};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_linear_gradient(g, ox, oy+30, ox, oy+CONTENT_H-20, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+20, oy+30, CONTENT_W-40, CONTENT_H-50, g);
    cevg_paint_destroy(g);
}

/* O133 — overlapping translucent strokes (ribbon) */
static void o133(CevgCanvas* cv, CevgTypeface* face) {
    int col=4,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O133");
    CevgPaint* p=cevg_paint_create(); cevg_paint_set_style(p,kCevgStyle_Stroke);
    cevg_paint_set_stroke_width(p, 22);
    for (int i=0;i<5;i++){
        CevgPath* path=cevg_path_create();
        cevg_path_move_to(path, ox+20, oy+50+i*8);
        cevg_path_quad_to(path, ox+CONTENT_W*0.5f, oy+150+i*8, ox+CONTENT_W-20, oy+50+i*8);
        cevg_paint_set_color(p, 0.5f+0.1f*i, 0.7f-0.1f*i, 0.9f, 0.4f);
        cevg_canvas_draw_path(cv, path, p);
        cevg_path_destroy(path);
    }
    cevg_paint_destroy(p);
}

/* O134 — overlap of fill + its own translucent stroke outline */
static void o134(CevgCanvas* cv, CevgTypeface* face) {
    int col=5,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O134");
    for (int i=0;i<3;i++){
        float cx=ox+60+i*45, cy=oy+CONTENT_H*0.5f;
        circ(cv, cx, cy, 40, 0.2f+i*0.3f, 0.6f, 0.9f-i*0.2f, 0.6f);
        CevgPaint* s=cevg_paint_create(); cevg_paint_set_style(s,kCevgStyle_Stroke);
        cevg_paint_set_stroke_width(s, 6); cevg_paint_set_color(s, 1,1,1,0.6f);
        cevg_canvas_draw_circle(cv, cx, cy, 40, s); cevg_paint_destroy(s);
    }
}

/* O135 — layered scrim gradients (vignette over content) */
static void o135(CevgCanvas* cv, CevgTypeface* face) {
    int col=6,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O135");
    grad_backdrop(cv, ox, oy);
    uint32_t rad[]={0x00000000,0xCC000000};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_radial_gradient(g, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, 130, rad, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+16, oy+24, CONTENT_W-32, CONTENT_H-48, g);
    cevg_paint_destroy(g);
}

/* O136 — overlapping multiply gradients (color mixing) */
static void o136(CevgCanvas* cv, CevgTypeface* face) {
    int col=7,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O136");
    rect(cv, ox+16,oy+24, CONTENT_W-32, CONTENT_H-48, 1,1,1,1.0f);
    uint32_t a[]={0xFFFF0000,0xFFFFFF00};
    CevgPaint* g1=cevg_paint_create();
    cevg_paint_set_linear_gradient(g1, ox+20,oy, ox+CONTENT_W-20,oy, a, NULL, 2, kCevgTile_Clamp);
    cevg_paint_set_blend_mode(g1, kCevgBlendMode_Multiply);
    cevg_canvas_draw_circle(cv, ox+95, oy+110, 60, g1); cevg_paint_destroy(g1);
    uint32_t b[]={0xFF00FFFF,0xFF0000FF};
    CevgPaint* g2=cevg_paint_create();
    cevg_paint_set_linear_gradient(g2, ox, oy+20, ox, oy+CONTENT_H-20, b, NULL, 2, kCevgTile_Clamp);
    cevg_paint_set_blend_mode(g2, kCevgBlendMode_Multiply);
    cevg_canvas_draw_circle(cv, ox+150, oy+130, 60, g2); cevg_paint_destroy(g2);
}

/* O137 — shadowed translucent glass pill over text-like bars */
static void o137(CevgCanvas* cv, CevgTypeface* face) {
    int col=8,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O137");
    for (int i=0;i<6;i++) rect(cv, ox+24, oy+40+i*26, CONTENT_W-48, 14, 0.6f,0.7f,0.9f,1.0f);
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp,3,4,4,0x99000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_color(g, 1,1,1,0.4f);
    cevg_canvas_draw_round_rect(cv, ox+40, oy+CONTENT_H*0.5f-22, CONTENT_W-80, 44, 22,22, g);
    cevg_paint_destroy(g);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O138 — overlap of color-dodge highlights (light leaks) */
static void o138(CevgCanvas* cv, CevgTypeface* face) {
    int col=9,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O138");
    rect(cv, ox+16,oy+24, CONTENT_W-32, CONTENT_H-48, 0.1f,0.12f,0.2f,1.0f);
    const float cc[3][3]={{0.5f,0.2f,0.1f},{0.1f,0.4f,0.2f},{0.2f,0.1f,0.5f}};
    for (int i=0;i<3;i++){
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, cc[i][0],cc[i][1],cc[i][2], 1.0f);
        cevg_paint_set_blend_mode(p, kCevgBlendMode_ColorDodge);
        float a=i*2.094f;
        cevg_canvas_draw_circle(cv, ox+CONTENT_W*0.5f+cosf(a)*35, oy+CONTENT_H*0.5f+sinf(a)*35, 55, p);
        cevg_paint_destroy(p);
    }
}

/* O139 — translucent overlapping bars chart with grid behind */
static void o139(CevgCanvas* cv, CevgTypeface* face) {
    int col=10,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O139");
    CevgPaint* gl=cevg_paint_create(); cevg_paint_set_color(gl,1,1,1,0.1f);
    for (int i=0;i<5;i++) cevg_canvas_draw_rect(cv, ox+20, oy+40+i*30, CONTENT_W-40, 1, gl);
    cevg_paint_destroy(gl);
    float hs[6]={60,110,80,140,95,120};
    for (int i=0;i<6;i++)
        rect(cv, ox+24+i*32, oy+CONTENT_H-30-hs[i], 26, hs[i],
             0.3f+0.1f*i, 0.7f, 1.0f-0.1f*i, 0.7f);
}

/* O140 — overlapping rotated translucent squares (kaleidoscope) */
static void o140(CevgCanvas* cv, CevgTypeface* face) {
    int col=11,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O140");
    cevg_canvas_save(cv); cevg_canvas_translate(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f);
    for (int i=0;i<8;i++){
        cevg_canvas_rotate(cv, 22.5f);
        CevgPaint* p=cevg_paint_create();
        cevg_paint_set_color(p, 0.5f+0.5f*sinf(i*0.8f), 0.4f, 0.5f+0.5f*cosf(i*0.8f), 0.35f);
        cevg_canvas_draw_rect(cv, -60, -60, 120, 120, p);
        cevg_paint_destroy(p);
    }
    cevg_canvas_restore(cv);
}

/* O141 — star clip with frosted + shadow ring (badge) */
static void o141(CevgCanvas* cv, CevgTypeface* face) {
    int col=12,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O141");
    busy_backdrop(cv, ox, oy);
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    float b[4]={ox+10,oy+16,ox+CONTENT_W-6,oy+CONTENT_H-6};
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_drop_shadow(lp,3,3,5,0xCC000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    cevg_canvas_save(cv);
    CevgPath* st=make_star(cx,cy,80,34);
    cevg_canvas_clip_path(cv,st);
    uint32_t cols[]={0xFFFFD040,0xFFFF8020};
    CevgPaint* g=cevg_paint_create();
    cevg_paint_set_radial_gradient(g, cx, cy, 80, cols, NULL, 2, kCevgTile_Clamp);
    cevg_canvas_draw_rect(cv, ox+20,oy+20, CONTENT_W-40, CONTENT_H-40, g); cevg_paint_destroy(g);
    cevg_canvas_restore(cv); cevg_path_destroy(st);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
}

/* O142 — overlapping translucent text-block bars + frosted header */
static void o142(CevgCanvas* cv, CevgTypeface* face) {
    int col=13,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O142");
    grad_backdrop(cv, ox, oy);
    float b[4]={ox,oy+24,ox+CONTENT_W,oy+78};
    cevg_canvas_save(cv); cevg_canvas_clip_rect(cv, ox, oy, CONTENT_W, CONTENT_H);
    CevgPaint* lp=cevg_paint_create(); cevg_paint_set_backdrop_blur(lp,10,10);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox, oy+24, CONTENT_W, 54, 0.1f,0.1f,0.18f,0.5f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    cevg_canvas_restore(cv);
    rect(cv, ox+24, oy+96, CONTENT_W-48, 12, 1,1,1,0.7f);
    rect(cv, ox+24, oy+118, CONTENT_W-80, 12, 1,1,1,0.45f);
    rect(cv, ox+24, oy+140, CONTENT_W-60, 12, 1,1,1,0.45f);
}

/* O143 — concentric clipped color wheel quarters overlapping */
static void o143(CevgCanvas* cv, CevgTypeface* face) {
    int col=14,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O143");
    float cx=ox+CONTENT_W*0.5f, cy=oy+CONTENT_H*0.5f;
    cevg_canvas_save(cv);
    CevgPath* cp=make_circle_path(cx,cy,85);
    cevg_canvas_clip_path(cv,cp);
    rect(cv, ox+8, oy+8, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.95f,0.3f,0.3f,0.85f);
    rect(cv, ox+CONTENT_W*0.5f, oy+8, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.3f,0.9f,0.4f,0.85f);
    rect(cv, ox+8, oy+CONTENT_H*0.5f, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.3f,0.4f,0.95f,0.85f);
    rect(cv, ox+CONTENT_W*0.5f, oy+CONTENT_H*0.5f, CONTENT_W*0.5f, CONTENT_H*0.5f, 0.95f,0.85f,0.2f,0.85f);
    circ(cv, cx, cy, 40, 1,1,1,0.6f);
    cevg_canvas_restore(cv); cevg_path_destroy(cp);
}

/* O144 — the works: backdrop, frosted card, clip, blend chip, shadow, text bars */
static void o144(CevgCanvas* cv, CevgTypeface* face) {
    int col=15,row=8; float ox,oy; cell_origin(col,row,&ox,&oy); draw_label(cv,face,col,row,"O144");
    /* gradient backdrop */
    grad_backdrop(cv, ox, oy);
    /* frosted card with shadow */
    float b[4]={ox+22,oy+38,ox+CONTENT_W-22,oy+CONTENT_H-30};
    CevgPaint* lp=cevg_paint_create();
    cevg_paint_set_backdrop_blur(lp, 10,10);
    cevg_paint_set_drop_shadow(lp, 3,4,5, 0xAA000000);
    cevg_canvas_save_layer(cv,1.0f,b,lp);
    rect(cv, ox+22,oy+38, CONTENT_W-44, CONTENT_H-68, 0.12f,0.14f,0.22f,0.5f);
    cevg_canvas_restore_layer(cv); cevg_paint_destroy(lp);
    /* blend chip */
    CevgPaint* chip=cevg_paint_create();
    cevg_paint_set_color(chip, 0.95f,0.8f,0.2f,1.0f);
    cevg_paint_set_blend_mode(chip, kCevgBlendMode_Screen);
    cevg_canvas_draw_round_rect(cv, ox+34, oy+50, 50, 26, 13,13, chip); cevg_paint_destroy(chip);
    /* clipped circle avatar */
    cevg_canvas_save(cv);
    CevgPath* cp=make_circle_path(ox+CONTENT_W-58, oy+64, 16);
    cevg_canvas_clip_path(cv,cp);
    rect(cv, ox+CONTENT_W-74, oy+48, 32,32, 0.3f,0.7f,0.95f,1.0f);
    cevg_canvas_restore(cv); cevg_path_destroy(cp);
    /* text bars */
    rect(cv, ox+36, oy+92, CONTENT_W-72, 9, 1,1,1,0.8f);
    rect(cv, ox+36, oy+110, CONTENT_W-100, 9, 1,1,1,0.5f);
    rect(cv, ox+36, oy+128, CONTENT_W-86, 9, 1,1,1,0.5f);
    /* translucent action button */
    CevgPaint* btn=cevg_paint_create(); cevg_paint_set_color(btn, 0.2f,0.6f,1.0f,0.9f);
    cevg_canvas_draw_round_rect(cv, ox+36, oy+CONTENT_H-58, 90, 26, 13,13, btn);
    cevg_paint_destroy(btn);
}

/* ================================================================== */
/* Cell filter: --only / --skip command-line selection                */
/* ================================================================== */

#define MAX_TESTS 144

typedef struct {
    const char* id;     /* "O001", "O002", ... */
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

/* Parse "O001" -> 0, "O012" -> 11, etc. Returns -1 on failure. */
static int parse_test_id(const char* s) {
    if (!s) return -1;
    if (s[0] != 'O' && s[0] != 'o') return -1;
    int n = atoi(s + 1);
    if (n < 1 || n > MAX_TESTS) return -1;
    return n - 1;
}

/* Parse comma-separated test IDs like "O001,O003,O005" */
static void parse_list(const char* list, int* out, int* count, int max) {
    *count = 0;
    char buf[256];
    strncpy(buf, list, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* tok = strtok(buf, ",");
    while (tok && *count < max) {
        int idx = parse_test_id(tok);
        if (idx >= 0) out[(*count)++] = idx;
        tok = strtok(NULL, ",");
    }
}

static void print_usage(const char* prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  --only O001,O003,... Run only these tests\n");
    printf("  --skip O002,O005,... Skip these tests\n");
    printf("  --list               List all test IDs and exit\n");
    printf("  --help               Show this help\n");
    printf("\nExamples:\n");
    printf("  %s --only O001,O006        Run only O001 and O006\n", prog);
    printf("  %s --skip O003,O007,O011   Run all except O003, O007, O011\n", prog);
}

/* ================================================================== */
/* Test table                                                         */
/* ================================================================== */

static const TestEntry g_tests[] = {
    { "O001", o001 },
    { "O002", o002 },
    { "O003", o003 },
    { "O004", o004 },
    { "O005", o005 },
    { "O006", o006 },
    { "O007", o007 },
    { "O008", o008 },
    { "O009", o009 },
    { "O010", o010 },
    { "O011", o011 },
    { "O012", o012 },
    { "O013", o013 },
    { "O014", o014 },
    { "O015", o015 },
    { "O016", o016 },
    { "O017", o017 },
    { "O018", o018 },
    { "O019", o019 },
    { "O020", o020 },
    { "O021", o021 },
    { "O022", o022 },
    { "O023", o023 },
    { "O024", o024 },
    { "O025", o025 },
    { "O026", o026 },
    { "O027", o027 },
    { "O028", o028 },
    { "O029", o029 },
    { "O030", o030 },
    { "O031", o031 },
    { "O032", o032 },
    { "O033", o033 },
    { "O034", o034 },
    { "O035", o035 },
    { "O036", o036 },
    { "O037", o037 },
    { "O038", o038 },
    { "O039", o039 },
    { "O040", o040 },
    { "O041", o041 },
    { "O042", o042 },
    { "O043", o043 },
    { "O044", o044 },
    { "O045", o045 },
    { "O046", o046 },
    { "O047", o047 },
    { "O048", o048 },
    { "O049", o049 },
    { "O050", o050 },
    { "O051", o051 },
    { "O052", o052 },
    { "O053", o053 },
    { "O054", o054 },
    { "O055", o055 },
    { "O056", o056 },
    { "O057", o057 },
    { "O058", o058 },
    { "O059", o059 },
    { "O060", o060 },
    { "O061", o061 },
    { "O062", o062 },
    { "O063", o063 },
    { "O064", o064 },
    { "O065", o065 },
    { "O066", o066 },
    { "O067", o067 },
    { "O068", o068 },
    { "O069", o069 },
    { "O070", o070 },
    { "O071", o071 },
    { "O072", o072 },
    { "O073", o073 },
    { "O074", o074 },
    { "O075", o075 },
    { "O076", o076 },
    { "O077", o077 },
    { "O078", o078 },
    { "O079", o079 },
    { "O080", o080 },
    { "O081", o081 },
    { "O082", o082 },
    { "O083", o083 },
    { "O084", o084 },
    { "O085", o085 },
    { "O086", o086 },
    { "O087", o087 },
    { "O088", o088 },
    { "O089", o089 },
    { "O090", o090 },
    { "O091", o091 },
    { "O092", o092 },
    { "O093", o093 },
    { "O094", o094 },
    { "O095", o095 },
    { "O096", o096 },
    { "O097", o097 },
    { "O098", o098 },
    { "O099", o099 },
    { "O100", o100 },
    { "O101", o101 },
    { "O102", o102 },
    { "O103", o103 },
    { "O104", o104 },
    { "O105", o105 },
    { "O106", o106 },
    { "O107", o107 },
    { "O108", o108 },
    { "O109", o109 },
    { "O110", o110 },
    { "O111", o111 },
    { "O112", o112 },
    { "O113", o113 },
    { "O114", o114 },
    { "O115", o115 },
    { "O116", o116 },
    { "O117", o117 },
    { "O118", o118 },
    { "O119", o119 },
    { "O120", o120 },
    { "O121", o121 },
    { "O122", o122 },
    { "O123", o123 },
    { "O124", o124 },
    { "O125", o125 },
    { "O126", o126 },
    { "O127", o127 },
    { "O128", o128 },
    { "O129", o129 },
    { "O130", o130 },
    { "O131", o131 },
    { "O132", o132 },
    { "O133", o133 },
    { "O134", o134 },
    { "O135", o135 },
    { "O136", o136 },
    { "O137", o137 },
    { "O138", o138 },
    { "O139", o139 },
    { "O140", o140 },
    { "O141", o141 },
    { "O142", o142 },
    { "O143", o143 },
    { "O144", o144 },
};
#define TEST_COUNT (sizeof(g_tests) / sizeof(g_tests[0]))

/* ================================================================== */
/* Main                                                                */
/* ================================================================== */

#ifndef CEVG_TEST_NO_MAIN
int main(int argc, char** argv) {
    printf("=== Cevg Overlap Test (O001-O144) ===\n");

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
    cevg_test_save_png("test_cevg_overlap.png");
    printf("PNG saved: test_cevg_overlap.png (%dx%d)\n", FB_W, FB_H);

    /* 7. Cleanup */
    if (face) cevg_typeface_unref(face);
    cevg_test_shutdown();

    printf("Done!\n");
    return 0;
}
#endif /* CEVG_TEST_NO_MAIN */

/* Exported for benchmark use. Draws the full O001-O144 overlap grid. */
void render_cevg_overlap_all(CevgCanvas* cv, CevgTypeface* face) {
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
