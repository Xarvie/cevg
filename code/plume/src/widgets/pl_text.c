/* pl_text.c (widget) — pl.Text：文本渲染（单行 / 省略号截断 / 多行换行）。
 * 单行：缓存全文 blob（文本/字号变才重塑）。
 * ellipsis：超宽时二分码点边界，取最长「前缀 + …」重塑（按 max_w 缓存，UTF-8 安全）。
 * wrap：超宽时用 find_breaks 拿候选断点，贪心装行，逐行塑形（按 max_w 缓存）。 */
#include "pl_text.h"
#include "text/pl_text.h"
#include "schema/plume_schema_gen.h"
#include <plume/plume_kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char*         text;
    float         size;
    PlColor       color;
    bool          ellipsis, wrap, ink_center;
    int           dir;                /* cevg 方向：0=LTR 1=RTL 2=Auto */
    int           base_rtl;           /* 本帧有效基方向是否 RTL（决定靠右对齐）*/
    float         line_height_ratio;  /* 行高比例（默认 1.2；line_h = size × ratio）*/
    float         letter_spacing;     /* 字间距 px（暂存，供外部读取；cevg API 限制暂不注入到 blob）*/
    CevgTextBlob* blob;          /* 全文 */
    float         w, h, ascent;
    /* ellipsis */
    CevgTextBlob* ell_blob; float ell_w; float ell_maxw;
    /* wrap */
    CevgTextBlob** lines; int n_lines; float wrap_w, line_h, wrap_maxw;
    /* 本帧模式 */
    int           mode;          /* 0=单行(blob)，1=省略号(ell_blob)，2=多行(lines) */
    const CevgTextBlob* active;
} TextData;

static void* text_create(void) {
    TextData* d = (TextData*)calloc(1, sizeof *d);
    if (d) { d->size = 16.0f; d->color = 0x000000FFu; d->ell_maxw = -1.0f; d->wrap_maxw = -1.0f; }
    return d;
}
static void free_lines(TextData* d) {
    for (int i = 0; i < d->n_lines; ++i) pl_text_release(d->lines[i]);
    free(d->lines); d->lines = NULL; d->n_lines = 0; d->wrap_maxw = -1.0f;
}
static void reset_ell(TextData* d) { pl_text_release(d->ell_blob); d->ell_blob = NULL; d->ell_maxw = -1.0f; }
static void text_destroy(PlRenderObj ro) {
    TextData* d = (TextData*)pl_ro_data(ro);
    if (d) { pl_text_release(d->blob); reset_ell(d); free_lines(d); free(d->text); free(d); }
}

/* UTF-8：从字节 i 前进一个码点 */
static int next_cp(const char* s, int len, int i) {
    if (i >= len) return len;
    i++; while (i < len && ((unsigned char)s[i] & 0xC0) == 0x80) i++;
    return i;
}
/* 「前缀(到 plen) + …」塑形 */
static CevgTextBlob* shape_prefix_ellipsis(const char* text, int plen, float size, int dir, float* ow, float* oh) {
    static const char E[] = "\xE2\x80\xA6";
    char* c = (char*)malloc((size_t)plen + sizeof E);
    memcpy(c, text, (size_t)plen); memcpy(c + plen, E, sizeof E);
    CevgTextBlob* b = pl_text_shape_dir(c, (size_t)plen + 3, size, dir);
    pl_text_measure(b, ow, oh); free(c); return b;
}
static CevgTextBlob* make_ellipsized(const char* text, float size, int dir, float max_w, float* ow, float* oh) {
    int len = (int)strlen(text), nb = 0;
    for (int i = 0;; ) { nb++; if (i >= len) break; i = next_cp(text, len, i); }
    int* bnd = (int*)malloc((size_t)nb * sizeof(int)); int m = 0;
    for (int i = 0;; ) { bnd[m++] = i; if (i >= len) break; i = next_cp(text, len, i); }
    int lo = 0, hi = m - 1, best = 0;
    while (lo <= hi) { int mid = (lo + hi) / 2; float w, h;
        CevgTextBlob* b = shape_prefix_ellipsis(text, bnd[mid], size, dir, &w, &h); pl_text_release(b);
        if (w <= max_w) { best = mid; lo = mid + 1; } else hi = mid - 1; }
    CevgTextBlob* r = shape_prefix_ellipsis(text, bnd[best], size, dir, ow, oh); free(bnd); return r;
}

/* 贪心换行：find_breaks 拿候选断点，逐行装满；硬换行(\n)强制断 */
static void compute_wrap(TextData* d, float max_w) {
    free_lines(d);
    int len = (int)strlen(d->text);
    int br[256]; int nb = pl_text_find_breaks(d->text, len, br, 255);
    br[nb++] = len;                                         /* 末尾边界 */
    d->line_h = pl_text_line_height(d->size);
    if (d->line_height_ratio > 0.0f) d->line_h = d->size * d->line_height_ratio;
    int cap = 8; d->lines = (CevgTextBlob**)malloc((size_t)cap * sizeof(CevgTextBlob*)); d->n_lines = 0;
    float maxw = 0; int start = 0, bi = 0;
    while (start < len) {
        int best = -1, k = bi;
        while (k < nb && br[k] <= start) k++;
        while (k < nb) {
            int c = br[k];
            CevgTextBlob* tb = pl_text_shape_dir(d->text + start, (size_t)(c - start), d->size, d->dir);
            float w, h; pl_text_measure(tb, &w, &h); pl_text_release(tb);
            bool hard = (c > 0 && c <= len && d->text[c - 1] == '\n');
            if (w <= max_w) { best = c; k++; if (hard) break; }
            else break;
        }
        if (best < 0) { best = (k < nb) ? br[k] : len; if (best <= start) best = len; }
        int rlen = best - start;                            /* 去尾部空白/换行再渲染 */
        while (rlen > 0 && (d->text[start + rlen - 1] == ' ' || d->text[start + rlen - 1] == '\n')) rlen--;
        CevgTextBlob* lb = pl_text_shape_dir(d->text + start, (size_t)rlen, d->size, d->dir);
        float lw, lh; pl_text_measure(lb, &lw, &lh); if (lw > maxw) maxw = lw;
        if (d->n_lines >= cap) { cap *= 2; d->lines = (CevgTextBlob**)realloc(d->lines, (size_t)cap * sizeof(CevgTextBlob*)); }
        d->lines[d->n_lines++] = lb;
        start = best; while (bi < nb && br[bi] <= start) bi++;
    }
    d->wrap_w = maxw; d->wrap_maxw = max_w;
}

static void reshape(TextData* d) {
    pl_text_release(d->blob); d->blob = NULL; d->w = d->h = 0.0f;
    reset_ell(d); free_lines(d);
    size_t tl = d->text ? strlen(d->text) : 0;
    if (tl) { d->blob = pl_text_shape_dir(d->text, tl, d->size, d->dir); pl_text_measure(d->blob, &d->w, &d->h); }
    /* 有效基方向：强制 LTR/RTL 直接用；Auto 则按首个强方向字符判定 */
    d->base_rtl = (d->dir==1) ? 1 : (d->dir==2 && tl) ? pl_text_base_dir(d->text, tl) : 0;
    d->ascent = pl_text_ascent(d->size);
}
static void text_update(PlRenderObj ro, PlProps p) {
    TextData* d = (TextData*)pl_ro_data(ro);
    const char* t = pl_props_str(p, kPlProp_Text, "");
    float sz = pl_props_f(p, kPlProp_FontSize, 16.0f);
    d->color          = pl_props_color(p, kPlProp_Color, 0x000000FFu);
    d->line_height_ratio = pl_props_f(p, kPlPropText_LineHeight,    0.0f);   /* 0=默认 */
    d->letter_spacing    = pl_props_f(p, kPlPropText_LetterSpacing, 0.0f);
    bool ell = pl_props_bool(p, kPlPropText_Ellipsis, false);
    bool wr  = pl_props_bool(p, kPlPropText_Wrap, false);
    d->ink_center = pl_props_bool(p, kPlPropText_InkCenter, false);
    int  pd  = pl_props_enum(p, kPlPropText_Dir, kPlDir_Auto);          /* 公共枚举 Auto=0/LTR=1/RTL=2 */
    int  cd  = (pd==kPlDir_LTR)?0 : (pd==kPlDir_RTL)?1 : 2;             /* → cevg 约定 LTR=0/RTL=1/Auto=2 */
    if (ell != d->ellipsis) { d->ellipsis = ell; reset_ell(d); }
    if (wr  != d->wrap)     { d->wrap = wr; free_lines(d); }
    bool changed = (d->text == NULL) ? (t[0] != 0) : (strcmp(d->text, t) != 0);
    if (changed || sz != d->size || cd != d->dir) {
        d->dir = cd;
        free(d->text); size_t n = strlen(t); d->text = (char*)malloc(n + 1); memcpy(d->text, t, n + 1);
        d->size = sz; reshape(d);
    }
}
static PlSize text_measure(PlRenderObj ro, PlConstraints c) {
    TextData* d = (TextData*)pl_ro_data(ro);
    /* 用字体级行高而非 blob ink height：cevg_text_blob_get_height 返回实际墨迹高度，
     * 随字符内容变化（CJK 比 ASCII 高 2-4px），导致 Row/Stack 居中时不同内容偏移不一致。
     * 行高 = |ascent|+descent 是字体级常量，保证同字号所有文本测量一致。 */
    float font_h = pl_text_line_height(d->size);
    float w = d->w, h = font_h; d->mode = 0; d->active = d->blob;
    bool over = (d->blob && c.max_w < PL_UNBOUNDED && d->w > c.max_w);
    if (d->wrap && over) {                                  /* 多行 */
        if (d->wrap_maxw != c.max_w) compute_wrap(d, c.max_w);
        float lh = (d->line_height_ratio>0)?d->size*d->line_height_ratio:d->line_h;
        d->mode = 2; w = d->wrap_w; h = (float)d->n_lines * lh;
    } else if (d->ellipsis && over) {                       /* 省略号 */
        if (d->ell_maxw != c.max_w) { reset_ell(d); d->ell_blob = make_ellipsized(d->text, d->size, d->dir, c.max_w, &d->ell_w, &h); d->ell_maxw = c.max_w; }
        d->mode = 1; d->active = d->ell_blob; w = d->ell_w; h = font_h;
    }
    /* RTL 段在【有界宽度】内铺满可用宽，从而能靠右对齐（同 CSS dir=rtl 块级）；无界宽度则保持内容宽。 */
    if (d->base_rtl && c.max_w < PL_UNBOUNDED && w < c.max_w) w = c.max_w;
    if (w < c.min_w) w = c.min_w;
    if (c.max_w < PL_UNBOUNDED && w > c.max_w) w = c.max_w;
    if (h < c.min_h) h = c.min_h;
    if (c.max_h < PL_UNBOUNDED && h > c.max_h) h = c.max_h;
    PlSize s = { w, h }; return s;
}
static void text_paint(PlRenderObj ro, PlCanvas cv) {
    TextData* d = (TextData*)pl_ro_data(ro);
    PlSize sz = pl_ro_size(ro);
    if (d->mode == 2) {                                     /* 多行：逐行画于各自基线 */
        float lh = (d->line_height_ratio>0)?d->size*d->line_height_ratio:d->line_h;
        for (int i = 0; i < d->n_lines; ++i) {
            float x = 0.0f;
            if (d->base_rtl) { float lw, lh2; pl_text_measure(d->lines[i], &lw, &lh2); x = sz.w - lw; }   /* RTL 行靠右 */
            pl_canvas_draw_text_blob(cv, d->lines[i], x, d->ascent + (float)i * lh, d->color);
        }
    } else {
        const CevgTextBlob* b = d->active ? d->active : d->blob;
        if (b) {
            float x = 0.0f;
            if (d->base_rtl) { float bw, bh; pl_text_measure(b, &bw, &bh); x = sz.w - bw; }                /* RTL 段靠右 */
            float y = d->ascent;                              /* 默认：基线 = ascent */
            if (d->ink_center) {                              /* 按墨迹盒垂直居中：补偿 CJK 字形上下偏移 */
                float ink[4]; cevg_text_blob_get_ink_bounds(b, ink);
                y = sz.h * 0.5f - (ink[1] + ink[3] * 0.5f);   /* 墨迹中心对齐 widget 中心 */
            }
            pl_canvas_draw_text_blob(cv, b, x, y, d->color);
        }
    }
}

static float text_baseline(PlRenderObj ro) {
    TextData* d = (TextData*)pl_ro_data(ro);
    return d->ascent;                                      /* 顶→基线 = ascent */
}

static const PlWidgetVTable kTextVTable = {
    .struct_size = sizeof(PlWidgetVTable), .type_name = "pl.Text",
    .build = NULL, .ro_create = text_create, .ro_update = text_update,
    .ro_measure = text_measure, .ro_arrange = NULL, .ro_paint = text_paint,
    .ro_hit = NULL, .ro_semantics = NULL, .ro_destroy = text_destroy,
    .ro_baseline = text_baseline,
};
void pl_text_register(void) { pl_register_widget(&kTextVTable); }
