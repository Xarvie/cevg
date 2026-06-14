#ifndef CEVG_INTERNAL_H
#define CEVG_INTERNAL_H

#include <cevg/cevg.h>
#include <stdlib.h>

/* ===================================================================
 * Internal structures shared between cevg_common.c (Path/Paint) and the
 * single Graphite backend (cevg_skia_graphite.cpp).
 *
 * Only CevgPath_ and CevgPaint_ live here, because they are pure CPU
 * data with no GPU dependency and are implemented once in cevg_common.c.
 * Every other handle (Context, Recorder, Surface, Canvas, Typeface,
 * TextBlob, Image, DisplayList) is defined privately inside the backend
 * translation unit.
 * =================================================================== */

/* ---- Path command tags ---- */
typedef enum {
    kCevgPathCmd_MoveTo,
    kCevgPathCmd_LineTo,
    kCevgPathCmd_QuadTo,
    kCevgPathCmd_CubicTo,
    kCevgPathCmd_Close,
} CevgPathCmd;

/* ---- Path (backend-agnostic) ---- */
struct CevgPath_ {
    CevgPathCmd* cmds;
    float*       coords;
    int          count;
    int          capacity;
    int          coord_count;
    int          coord_capacity;
    CevgFillRule fill_rule;      /* default kCevgFillRule_NonZero */
    uint32_t     generation;     /* bumped on every mutation (tessellation cache key) */
    uint32_t     uid;            /* unique, never recycled (stable cache key) */
};

/* ---- Paint (backend-agnostic) ----
 * The generation/backend_cache fields let the GPU backend memoize the
 * resolved SkPaint (and its shader/colorfilter/imagefilter) so a paint
 * that does not change between draws is converted exactly once. */
struct CevgPaint_ {
    /* Basic */
    float          color[4];          /* [r, g, b, a], non-premultiplied */
    CevgPaintStyle style;
    float          alpha;
    bool           anti_alias;
    CevgBlendMode  blend_mode;

    /* Stroke */
    float    stroke_width;
    CevgCap  stroke_cap;
    CevgJoin stroke_join;
    float    stroke_miter;

    /* Dash */
    float*  dash_pattern;
    int     dash_count;
    float   dash_phase;

    /* Gradient */
    bool         has_linear_gradient;
    bool         has_radial_gradient;
    float        grad_pts[4];         /* [x0,y0,x1,y1] or [cx,cy,r,_] */
    uint32_t*    grad_colors;
    float*       grad_stops;
    int          grad_count;
    CevgTileMode grad_tile;

    /* Image shader (reference-counted; the paint holds one reference while
     * has_image_shader is set — see cevg_image_ref/unref below) */
    bool         has_image_shader;
    CevgImage*   image_shader;
    CevgTileMode image_tile_x;
    CevgTileMode image_tile_y;

    /* Image filter */
    bool      has_blur;
    float     blur_sigma_x;
    float     blur_sigma_y;
    bool      has_drop_shadow;
    float     shadow_dx;
    float     shadow_dy;
    float     shadow_sigma;
    uint32_t  shadow_color;
    bool      has_color_matrix;
    float     color_matrix[20];

    /* Sampling */
    CevgFilterQuality filter_quality;

    /* Backdrop filter (for the next save_layer) */
    bool      has_backdrop_blur;
    float     backdrop_blur_sigma_x;
    float     backdrop_blur_sigma_y;
    bool      has_backdrop_shadow;
    float     backdrop_shadow_dx;
    float     backdrop_shadow_dy;
    float     backdrop_shadow_sigma;
    uint32_t  backdrop_shadow_color;

    /* Backend resolved-paint cache */
    uint32_t  generation;                  /* bumped on every set_* */
    void*     backend_cache;               /* opaque SkPaintCacheEntry* */
    void      (*backend_cache_dtor)(void*);/* frees backend_cache */
};

/* ---- Internal image lifetime (implemented in the backend) ----
 * CevgImage is reference counted so a CevgPaint can safely hold an image
 * shader even if the caller destroys its own handle first. The public
 * cevg_image_destroy() drops the caller's reference; these add/drop the
 * extra reference a paint holds. Declared here (not in the public header)
 * so cevg_common.c can retain/release the referenced image without seeing
 * CevgImage's private layout. */
#ifdef __cplusplus
extern "C" {
#endif
void cevg_image_ref(CevgImage* image);
void cevg_image_unref(CevgImage* image);
#ifdef __cplusplus
}
#endif

#endif /* CEVG_INTERNAL_H */
