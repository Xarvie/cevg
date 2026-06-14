#include "cevg_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <limits.h>

/* ===================================================================
 * cevg_common.c — Backend-agnostic Path & Paint implementation
 * ===================================================================
 * Context, Surface, Canvas, Typeface, TextBlob, Image are implemented
 * by the backend file (backend_gles3.c or backend_skia.cpp).
 * This file only contains Path and Paint, which are shared.
 * =================================================================== */

/* ===================================================================
 * Version / Backend name — delegated to backend
 * ===================================================================
 * These are implemented in the backend file.
 * extern const char* cevg_version(void);
 * extern const char* cevg_backend_name(void);
 */

/* ===================================================================
 * Path
 * =================================================================== */
#define CEVG_PATH_INIT_CAPACITY 16

static atomic_uint cevg_path_next_uid = 1;

CevgPath* cevg_path_create(void) {
    CevgPath* path = (CevgPath*)calloc(1, sizeof(CevgPath));
    if (!path) return NULL;
    path->capacity = CEVG_PATH_INIT_CAPACITY;
    path->cmds = (CevgPathCmd*)malloc(path->capacity * sizeof(CevgPathCmd));
    path->coord_capacity = CEVG_PATH_INIT_CAPACITY * 6;
    path->coords = (float*)malloc(path->coord_capacity * sizeof(float));
    if (!path->cmds || !path->coords) {
        free(path->cmds);
        free(path->coords);
        free(path);
        return NULL;
    }
    path->uid = atomic_fetch_add(&cevg_path_next_uid, 1);
    return path;
}

void cevg_path_destroy(CevgPath* path) {
    if (!path) return;
    free(path->cmds);
    free(path->coords);
    free(path);
}

/* Path capacity is stored in int. Guard the doubling against integer overflow:
 * a browser feeds this untrusted SVG path data, so the command/coord counts can
 * be adversarial. On overflow or allocation failure we fail the grow; the caller
 * (cevg_path_add_cmd) then drops the command rather than under-allocating and
 * corrupting the heap. */
#define CEVG_PATH_MAX_CMDS   (INT_MAX / (int)sizeof(CevgPathCmd))
#define CEVG_PATH_MAX_COORDS (INT_MAX / (int)sizeof(float))

static bool cevg_path_grow(CevgPath* path) {
    if (path->capacity >= CEVG_PATH_MAX_CMDS) return false;  /* cannot grow further */
    int new_cap = (path->capacity > CEVG_PATH_MAX_CMDS / 2)
                  ? CEVG_PATH_MAX_CMDS
                  : (path->capacity > 0 ? path->capacity * 2 : CEVG_PATH_INIT_CAPACITY);
    CevgPathCmd* new_cmds =
        (CevgPathCmd*)realloc(path->cmds, (size_t)new_cap * sizeof(CevgPathCmd));
    if (!new_cmds) return false;
    path->cmds = new_cmds;
    path->capacity = new_cap;
    return true;
}

static bool cevg_path_grow_coords(CevgPath* path, int needed) {
    if (needed < 0) return false;
    /* Already enough? Compare via subtraction so coord_count+needed can't overflow. */
    if (needed <= path->coord_capacity - path->coord_count) return true;
    if (needed > CEVG_PATH_MAX_COORDS - path->coord_count) return false;  /* would overflow int */
    int required = path->coord_count + needed;
    int new_cap = path->coord_capacity > 0 ? path->coord_capacity : CEVG_PATH_INIT_CAPACITY * 6;
    while (new_cap < required) {
        if (new_cap > CEVG_PATH_MAX_COORDS / 2) { new_cap = required; break; }
        new_cap *= 2;
    }
    float* new_coords =
        (float*)realloc(path->coords, (size_t)new_cap * sizeof(float));
    if (!new_coords) return false;
    path->coords = new_coords;
    path->coord_capacity = new_cap;
    return true;
}

static void cevg_path_add_cmd(CevgPath* path, CevgPathCmd cmd,
                              const float* coords, int ncoords) {
    if (path->count >= path->capacity) {
        if (!cevg_path_grow(path)) return;
    }
    if (!cevg_path_grow_coords(path, ncoords)) return;
    path->cmds[path->count++] = cmd;
    if (ncoords > 0 && coords) {
        memcpy(&path->coords[path->coord_count], coords, ncoords * sizeof(float));
    }
    path->coord_count += ncoords;
    path->generation++;
}

void cevg_path_move_to(CevgPath* path, float x, float y) {
    if (!path) return;
    float coords[] = {x, y};
    cevg_path_add_cmd(path, kCevgPathCmd_MoveTo, coords, 2);
}

void cevg_path_line_to(CevgPath* path, float x, float y) {
    if (!path) return;
    float coords[] = {x, y};
    cevg_path_add_cmd(path, kCevgPathCmd_LineTo, coords, 2);
}

void cevg_path_quad_to(CevgPath* path, float cx, float cy, float x, float y) {
    if (!path) return;
    float coords[] = {cx, cy, x, y};
    cevg_path_add_cmd(path, kCevgPathCmd_QuadTo, coords, 4);
}

void cevg_path_cubic_to(CevgPath* path, float c1x, float c1y,
                        float c2x, float c2y, float x, float y) {
    if (!path) return;
    float coords[] = {c1x, c1y, c2x, c2y, x, y};
    cevg_path_add_cmd(path, kCevgPathCmd_CubicTo, coords, 6);
}

void cevg_path_close(CevgPath* path) {
    if (!path) return;
    cevg_path_add_cmd(path, kCevgPathCmd_Close, NULL, 0);
}

/* ===================================================================
 * Paint
 * =================================================================== */
/* Defined further below; used by the setters that replace/clear the shader
 * (declared up here because several of them precede the definition). */
static void cevg_paint_drop_image_shader(CevgPaint* paint);

CevgPaint* cevg_paint_create(void) {
    CevgPaint* paint = (CevgPaint*)calloc(1, sizeof(CevgPaint));
    if (!paint) return NULL;
    /* Defaults — per design doc: black, fill, SrcOver, etc. */
    paint->color[0] = 0.0f;
    paint->color[1] = 0.0f;
    paint->color[2] = 0.0f;
    paint->color[3] = 1.0f;
    paint->style   = kCevgStyle_Fill;
    paint->alpha   = 1.0f;
    paint->anti_alias = true;
    paint->blend_mode  = kCevgBlendMode_SrcOver;
    paint->stroke_width = 1.0f;
    paint->stroke_cap   = kCevgCap_Butt;
    paint->stroke_join  = kCevgJoin_Miter;
    paint->stroke_miter = 4.0f;
    paint->filter_quality = kCevgFilterQuality_Linear;
    return paint;
}

void cevg_paint_destroy(CevgPaint* paint) {
    if (!paint) return;
    if (paint->backend_cache && paint->backend_cache_dtor) {
        paint->backend_cache_dtor(paint->backend_cache);
    }
    cevg_paint_drop_image_shader(paint);   /* release the image shader, if any */
    free(paint->dash_pattern);
    free(paint->grad_colors);
    free(paint->grad_stops);
    free(paint);
}

void cevg_paint_reset(CevgPaint* paint) {
    if (!paint) return;
    /* Free backend cache */
    if (paint->backend_cache && paint->backend_cache_dtor) {
        paint->backend_cache_dtor(paint->backend_cache);
    }
    /* Release image shader before we zero the struct (else the ref leaks). */
    cevg_paint_drop_image_shader(paint);
    /* Free dynamic arrays */
    free(paint->dash_pattern);
    free(paint->grad_colors);
    free(paint->grad_stops);
    /* Zero everything, then set defaults (same as cevg_paint_create) */
    memset(paint, 0, sizeof(*paint));
    paint->color[0] = 0.0f;
    paint->color[1] = 0.0f;
    paint->color[2] = 0.0f;
    paint->color[3] = 1.0f;
    paint->style   = kCevgStyle_Fill;
    paint->alpha   = 1.0f;
    paint->anti_alias = true;
    paint->blend_mode  = kCevgBlendMode_SrcOver;
    paint->stroke_width = 1.0f;
    paint->stroke_cap   = kCevgCap_Butt;
    paint->stroke_join  = kCevgJoin_Miter;
    paint->stroke_miter = 4.0f;
    paint->filter_quality = kCevgFilterQuality_Linear;
    /* generation = 0, backend_cache = NULL, backend_cache_dtor = NULL via memset */
}

/* ---- Basic setters ---- */
void cevg_paint_set_color(CevgPaint* paint, float r, float g, float b, float a) {
    if (!paint) return;
    paint->color[0] = r; paint->color[1] = g;
    paint->color[2] = b; paint->color[3] = a;
    /* Setting a solid color replaces any previously installed shader, mirroring
     * the gradient setters (which already clear the other shader flags). Without
     * this, reusing one paint object for a gradient and then a solid fill would
     * silently keep painting the stale gradient — the solid color would be
     * ignored. "Last fill setter wins" is the least-surprising behaviour and
     * keeps a single reused paint usable for mixed fills. */
    paint->has_linear_gradient = false;
    paint->has_radial_gradient = false;
    cevg_paint_drop_image_shader(paint);
    paint->generation++;
}

void cevg_paint_set_style(CevgPaint* paint, CevgPaintStyle style) {
    if (!paint) return;
    paint->style = style;
    paint->generation++;
}

void cevg_paint_set_alpha(CevgPaint* paint, float alpha) {
    if (!paint) return;
    paint->alpha = alpha;
    paint->generation++;
}

void cevg_paint_set_anti_alias(CevgPaint* paint, bool enabled) {
    if (!paint) return;
    paint->anti_alias = enabled;
    paint->generation++;
}

void cevg_paint_set_blend_mode(CevgPaint* paint, CevgBlendMode mode) {
    if (!paint) return;
    paint->blend_mode = mode;
    paint->generation++;
}

/* ---- Stroke ---- */
void cevg_paint_set_stroke_width(CevgPaint* paint, float width) {
    if (!paint) return;
    paint->stroke_width = width;
    paint->generation++;
}

void cevg_paint_set_stroke_cap(CevgPaint* paint, CevgCap cap) {
    if (!paint) return;
    paint->stroke_cap = cap;
    paint->generation++;
}

void cevg_paint_set_stroke_join(CevgPaint* paint, CevgJoin join) {
    if (!paint) return;
    paint->stroke_join = join;
    paint->generation++;
}

void cevg_paint_set_stroke_miter(CevgPaint* paint, float limit) {
    if (!paint) return;
    paint->stroke_miter = limit;
    paint->generation++;
}

void cevg_paint_set_dash(CevgPaint* paint, const float* dashes, int count, float phase) {
    if (!paint || !dashes || count <= 0) return;
    if (count > INT_MAX / (int)sizeof(float)) return;           /* overflow guard */
    float* nd = (float*)malloc((size_t)count * sizeof(float));
    if (!nd) return;                          /* OOM: leave the existing dash intact */
    memcpy(nd, dashes, (size_t)count * sizeof(float));
    free(paint->dash_pattern);
    paint->dash_pattern = nd;
    paint->dash_count = count;
    paint->dash_phase = phase;
    paint->generation++;
}

/* ---- Gradient ---- */
void cevg_paint_set_linear_gradient(CevgPaint* paint, float x0, float y0,
                                    float x1, float y1,
                                    const uint32_t* colors, const float* stops,
                                    int count, CevgTileMode tile) {
    if (!paint || !colors || count <= 0) return;
    if (count > INT_MAX / (int)sizeof(uint32_t)) return;           /* overflow guard */
    /* Allocate + fill into temporaries FIRST. Only commit once everything has
     * succeeded, so an allocation failure leaves the paint's previous shader
     * fully intact instead of half-set (has_linear_gradient=true but with a
     * NULL/short array and a stale grad_count, which the backend would then
     * read out of bounds). */
    uint32_t* nc = (uint32_t*)malloc((size_t)count * sizeof(uint32_t));
    float*    ns = (float*)malloc((size_t)count * sizeof(float));
    if (!nc || !ns) { free(nc); free(ns); return; }
    memcpy(nc, colors, (size_t)count * sizeof(uint32_t));
    if (stops) memcpy(ns, stops, (size_t)count * sizeof(float));
    else for (int i = 0; i < count; i++) ns[i] = (count > 1) ? (float)i / (count - 1) : 0.0f;

    free(paint->grad_colors);
    free(paint->grad_stops);
    paint->grad_colors = nc;
    paint->grad_stops  = ns;
    paint->grad_count  = count;
    paint->grad_tile   = tile;
    paint->grad_pts[0] = x0; paint->grad_pts[1] = y0;
    paint->grad_pts[2] = x1; paint->grad_pts[3] = y1;
    paint->has_linear_gradient = true;
    paint->has_radial_gradient = false;
    cevg_paint_drop_image_shader(paint);
    paint->generation++;
}

void cevg_paint_set_radial_gradient(CevgPaint* paint, float cx, float cy,
                                    float radius,
                                    const uint32_t* colors, const float* stops,
                                    int count, CevgTileMode tile) {
    if (!paint || !colors || count <= 0) return;
    if (count > INT_MAX / (int)sizeof(uint32_t)) return;           /* overflow guard */
    uint32_t* nc = (uint32_t*)malloc((size_t)count * sizeof(uint32_t));
    float*    ns = (float*)malloc((size_t)count * sizeof(float));
    if (!nc || !ns) { free(nc); free(ns); return; }   /* OOM: leave paint unchanged */
    memcpy(nc, colors, (size_t)count * sizeof(uint32_t));
    if (stops) memcpy(ns, stops, (size_t)count * sizeof(float));
    else for (int i = 0; i < count; i++) ns[i] = (count > 1) ? (float)i / (count - 1) : 0.0f;

    free(paint->grad_colors);
    free(paint->grad_stops);
    paint->grad_colors = nc;
    paint->grad_stops  = ns;
    paint->grad_count  = count;
    paint->grad_tile   = tile;
    paint->grad_pts[0] = cx; paint->grad_pts[1] = cy;
    paint->grad_pts[2] = radius;
    paint->has_linear_gradient = false;
    paint->has_radial_gradient = true;
    cevg_paint_drop_image_shader(paint);
    paint->generation++;
}

/* Release the image-shader reference this paint holds (if any) and clear the
 * fields. cevg_image_unref is implemented in the backend (see cevg_internal.h). */
static void cevg_paint_drop_image_shader(CevgPaint* paint) {
    if (paint->has_image_shader && paint->image_shader) {
        cevg_image_unref(paint->image_shader);
    }
    paint->has_image_shader = false;
    paint->image_shader = NULL;
}

void cevg_paint_clear_shader(CevgPaint* paint) {
    if (!paint) return;
    paint->has_linear_gradient = false;
    paint->has_radial_gradient = false;
    cevg_paint_drop_image_shader(paint);
    free(paint->grad_colors); paint->grad_colors = NULL;
    free(paint->grad_stops);  paint->grad_stops  = NULL;
    paint->grad_count = 0;
    paint->generation++;
}

/* ---- Image shader ---- */
void cevg_paint_set_image_shader(CevgPaint* paint, CevgImage* image,
                                 CevgTileMode tile_x, CevgTileMode tile_y) {
    if (!paint || !image) return;
    cevg_image_ref(image);                 /* paint's reference to the new image */
    cevg_paint_drop_image_shader(paint);   /* release any previous one (ref-then-drop
                                            * is safe even if image == the old one) */
    /* Clear any existing gradient shader */
    paint->has_linear_gradient = false;
    paint->has_radial_gradient = false;
    free(paint->grad_colors); paint->grad_colors = NULL;
    free(paint->grad_stops);  paint->grad_stops  = NULL;
    paint->grad_count = 0;
    /* Set image shader */
    paint->has_image_shader = true;
    paint->image_shader = image;
    paint->image_tile_x = tile_x;
    paint->image_tile_y = tile_y;
    paint->generation++;
}

/* ---- Image filter ---- */
void cevg_paint_set_blur(CevgPaint* paint, float sigma_x, float sigma_y) {
    if (!paint) return;
    paint->has_blur = true;
    paint->blur_sigma_x = sigma_x;
    paint->blur_sigma_y = sigma_y;
    paint->generation++;
}

void cevg_paint_set_drop_shadow(CevgPaint* paint, float dx, float dy,
                                float sigma, uint32_t color) {
    if (!paint) return;
    paint->has_drop_shadow = true;
    paint->shadow_dx = dx;
    paint->shadow_dy = dy;
    paint->shadow_sigma = sigma;
    paint->shadow_color = color;
    paint->generation++;
}

void cevg_paint_set_color_matrix(CevgPaint* paint, const float matrix[20]) {
    if (!paint || !matrix) return;
    paint->has_color_matrix = true;
    memcpy(paint->color_matrix, matrix, 20 * sizeof(float));
    paint->generation++;
}

void cevg_paint_set_filter_quality(CevgPaint* paint, CevgFilterQuality quality) {
    if (!paint) return;
    paint->filter_quality = quality;
    paint->generation++;
}

void cevg_paint_clear_filter(CevgPaint* paint) {
    if (!paint) return;
    paint->has_blur = false;
    paint->has_drop_shadow = false;
    paint->has_color_matrix = false;
    paint->generation++;
}

/* ---- Backdrop filter ---- */
void cevg_paint_set_backdrop_blur(CevgPaint* paint, float sigma_x, float sigma_y) {
    if (!paint) return;
    paint->has_backdrop_blur = true;
    paint->backdrop_blur_sigma_x = sigma_x;
    paint->backdrop_blur_sigma_y = sigma_y;
    paint->generation++;
}

void cevg_paint_set_backdrop_shadow(CevgPaint* paint, float dx, float dy,
                                    float sigma, uint32_t color) {
    if (!paint) return;
    paint->has_backdrop_shadow = true;
    paint->backdrop_shadow_dx = dx;
    paint->backdrop_shadow_dy = dy;
    paint->backdrop_shadow_sigma = sigma;
    paint->backdrop_shadow_color = color;
    paint->generation++;
}

void cevg_paint_clear_backdrop(CevgPaint* paint) {
    if (!paint) return;
    paint->has_backdrop_blur = false;
    paint->has_backdrop_shadow = false;
    paint->generation++;
}

/* ---- Color ARGB ---- */
void cevg_paint_set_color_argb(CevgPaint* paint, uint32_t argb) {
    if (!paint) return;
    paint->color[0] = ((argb >> 16) & 0xFF) / 255.0f;
    paint->color[1] = ((argb >>  8) & 0xFF) / 255.0f;
    paint->color[2] = ((argb      ) & 0xFF) / 255.0f;
    paint->color[3] = ((argb >> 24) & 0xFF) / 255.0f;
    /* See cevg_paint_set_color: a solid color replaces any shader. */
    paint->has_linear_gradient = false;
    paint->has_radial_gradient = false;
    cevg_paint_drop_image_shader(paint);
    paint->generation++;
}

/* ---- Clear dash ---- */
void cevg_paint_clear_dash(CevgPaint* paint) {
    if (!paint) return;
    free(paint->dash_pattern);
    paint->dash_pattern = NULL;
    paint->dash_count = 0;
    paint->dash_phase = 0.0f;
    paint->generation++;
}

/* ===================================================================
 * Path — additional functions
 * =================================================================== */
void cevg_path_rewind(CevgPath* path) {
    if (!path) return;
    path->count = 0;
    path->coord_count = 0;
    path->generation++;
    /* Keep allocated buffers for reuse */
}

void cevg_path_get_bounds(const CevgPath* path, float rect[4]) {
    if (!path || !rect) return;
    if (path->count == 0) {
        rect[0] = rect[1] = rect[2] = rect[3] = 0.0f;
        return;
    }
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    int ci = 0;
    for (int i = 0; i < path->count; i++) {
        int ncoords = 0;
        switch (path->cmds[i]) {
            case kCevgPathCmd_MoveTo:
            case kCevgPathCmd_LineTo:
                ncoords = 2; break;
            case kCevgPathCmd_QuadTo:
                ncoords = 4; break;
            case kCevgPathCmd_CubicTo:
                ncoords = 6; break;
            case kCevgPathCmd_Close:
                ncoords = 0; break;
        }
        /* Visit all coordinate pairs in this command */
        for (int j = 0; j < ncoords; j += 2) {
            float x = path->coords[ci + j];
            float y = path->coords[ci + j + 1];
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
        ci += ncoords;
    }
    rect[0] = minX;
    rect[1] = minY;
    rect[2] = maxX - minX;
    rect[3] = maxY - minY;
}

/* ---- Fill rule (§4.5-B even-odd + §7 clip_path) ---- */
void cevg_path_set_fill_rule(CevgPath* path, CevgFillRule rule) {
    if (!path) return;
    path->fill_rule = rule;
    path->generation++;
}
CevgFillRule cevg_path_get_fill_rule(const CevgPath* path) {
    return path ? path->fill_rule : kCevgFillRule_NonZero;
}
