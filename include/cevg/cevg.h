/* =====================================================================
 * cevg.h — Cevg Vector Graphics public API
 * ---------------------------------------------------------------------
 * One shipping backend today: Skia's modern GPU engine (Graphite) on
 * Vulkan. The legacy GLES3 backend and the "two backends must match
 * pixel-for-pixel" contract are gone.
 *
 * Although only Vulkan is implemented now, the PUBLIC ABI is deliberately
 * backend-neutral: the GPU device is an opaque handle created by a
 * backend-specific factory (cevg_gpu_device_create_vulkan), so a future
 * Metal/Dawn/WebGPU backend can be added without changing — or breaking —
 * this header for existing binaries. See "ABI STABILITY CONTRACT" below.
 *
 * The C ABI (names, signatures, parameter order, enum values, struct
 * layout) is preserved from the previous Cevg VG header so existing
 * callers keep linking unchanged. What changed is the engine binding
 * underneath and a few entry points that only made sense for the old
 * CPU-readback present path (now removed) or the GLES3 native-FBO wrap
 * (now removed). New entry points were added for GPU-direct presentation
 * and retained display lists; they are grouped and clearly marked.
 *
 * Conventions that apply everywhere unless a function says otherwise:
 *   - Coordinate system: top-left origin, +X right, +Y down (Skia
 *     default). All coordinates are in the canvas' current user space
 *     unless explicitly noted as "device space".
 *   - Affine matrix: column-major 3x3, 9 floats laid out as
 *     [sx, ky, 0,  kx, sy, 0,  tx, ty, 1] (col0, col1, col2).
 *     A point transforms as x' = sx*x + kx*y + tx,
 *                           y' = ky*x + sy*y + ty.  No perspective.
 *   - Surface pixels: RGBA_8888, premultiplied alpha, sRGB color space.
 *   - Pointer arguments: every entry point NULL-checks its handle(s)
 *     and is a safe no-op (or returns 0 / an error code) on NULL.
 * ===================================================================== */
#ifndef CEVG_H
#define CEVG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Export macros ---- */
#if defined(_WIN32) && defined(CEVG_SHARED)
    #ifdef CEVG_EXPORTS
        #define CEVG_API __declspec(dllexport)
    #else
        #define CEVG_API __declspec(dllimport)
    #endif
#else
    #define CEVG_API
#endif

/* ---- Opaque handles ----
 * Each handle is owned by the backend; its internal layout is private
 * (defined in the backend translation unit) EXCEPT CevgPath_ and
 * CevgPaint_, whose bodies live in the shared cevg_internal.h and are
 * implemented once in cevg_common.c (backend-agnostic). */
typedef struct CevgContext_     CevgContext;
typedef struct CevgSurface_     CevgSurface;
typedef struct CevgCanvas_      CevgCanvas;
typedef struct CevgPath_        CevgPath;
typedef struct CevgPaint_       CevgPaint;
typedef struct CevgTypeface_    CevgTypeface;
typedef struct CevgTextBlob_    CevgTextBlob;
typedef struct CevgImage_       CevgImage;
typedef struct CevgDisplayList_ CevgDisplayList;  /* NEW: retained recording */
typedef struct CevgRecording_   CevgRecording;    /* NEW: snapped, immutable recording */

/* ---- Enumerations ---- */

/* Result / error codes. */
typedef enum {
    kCevgSuccess            = 0,
    kCevgErrorInvalidArg    = -1,  /* NULL/empty/out-of-range argument */
    kCevgErrorOOM           = -2,  /* allocation or GPU readback failure */
    kCevgErrorUnsupported   = -3,  /* operation not available */
    kCevgErrorFileNotFound  = -4,  /* image file could not be opened */
    kCevgErrorDecodeFailed  = -5,  /* image bytes could not be decoded */
    kCevgErrorDeviceLost    = -6,  /* NEW: GPU device/swapchain lost — call cevg_context_reset */
} CevgResult;

/* Paint style. Default kCevgStyle_Fill. */
typedef enum {
    kCevgStyle_Fill   = 0,
    kCevgStyle_Stroke = 1,
} CevgPaintStyle;

/* Stroke end-cap. Default kCevgCap_Butt. */
typedef enum {
    kCevgCap_Butt   = 0,
    kCevgCap_Round  = 1,
    kCevgCap_Square = 2,
} CevgCap;

/* Stroke line-join. Default kCevgJoin_Miter. */
typedef enum {
    kCevgJoin_Miter = 0,
    kCevgJoin_Round = 1,
    kCevgJoin_Bevel = 2,
} CevgJoin;

/* Compositing / blend modes. EXACTLY 29 entries mapping 1:1 onto
 * SkBlendMode. Out-of-range -> SrcOver. */
typedef enum {
    kCevgBlendMode_Clear = 0,  kCevgBlendMode_Src,       kCevgBlendMode_Dst,
    kCevgBlendMode_SrcOver,    kCevgBlendMode_DstOver,   kCevgBlendMode_SrcIn,
    kCevgBlendMode_DstIn,      kCevgBlendMode_SrcOut,    kCevgBlendMode_DstOut,
    kCevgBlendMode_SrcATop,    kCevgBlendMode_DstATop,   kCevgBlendMode_Xor,
    kCevgBlendMode_Plus,       kCevgBlendMode_Modulate,  kCevgBlendMode_Screen,
    kCevgBlendMode_Overlay,    kCevgBlendMode_Darken,    kCevgBlendMode_Lighten,
    kCevgBlendMode_ColorDodge, kCevgBlendMode_ColorBurn, kCevgBlendMode_HardLight,
    kCevgBlendMode_SoftLight,  kCevgBlendMode_Difference,kCevgBlendMode_Exclusion,
    kCevgBlendMode_Multiply,   kCevgBlendMode_Hue,       kCevgBlendMode_Saturation,
    kCevgBlendMode_Color,      kCevgBlendMode_Luminosity,
} CevgBlendMode;

/* Tiling for gradients and image shaders. Unknown -> Clamp. */
typedef enum {
    kCevgTile_Clamp  = 0,
    kCevgTile_Repeat = 1,
    kCevgTile_Mirror = 2,
} CevgTileMode;

/* Image sampling quality. Default Linear. */
typedef enum {
    kCevgFilterQuality_Nearest = 0,
    kCevgFilterQuality_Linear  = 1,
    kCevgFilterQuality_High    = 2,  /* bicubic Mitchell-Netravali (1/3,1/3) */
} CevgFilterQuality;

/* Working color space hint. sRGB is the default and the only one with
 * guaranteed stable output. */
typedef enum {
    kCevgColorSpace_sRGB   = 0,
    kCevgColorSpace_Linear = 1,
} CevgColorSpace;

/* Base text direction passed to the shaper. */
typedef enum {
    kCevgDir_LTR  = 0,
    kCevgDir_RTL  = 1,
    kCevgDir_Auto = 2,
} CevgTextDirection;

/* Path fill rule. */
typedef enum {
    kCevgFillRule_NonZero = 0,
    kCevgFillRule_EvenOdd = 1,
} CevgFillRule;

/* Present mode for window surfaces, described by SEMANTICS (not by any one
 * backend's vocabulary), so a future non-Vulkan backend can map these same
 * three meanings. The names are Cevg's permanent terms:
 *   Fifo      — vsync, never tears, always available. The safe default.
 *   Mailbox   — vsync but low-latency (newest frame wins; no tearing).
 *   Immediate — lowest latency, may tear.
 * A backend that cannot honor a mode falls back to Fifo. New modes may be
 * appended with new integer values; existing values never change meaning. */
typedef enum {
    kCevgPresentMode_Fifo      = 0,
    kCevgPresentMode_Mailbox   = 1,
    kCevgPresentMode_Immediate = 2,
} CevgPresentMode;

/* =====================================================================
 * ABI STABILITY CONTRACT  (READ BEFORE CHANGING ANYTHING IN THIS FILE)
 * ---------------------------------------------------------------------
 * This library is distributed as a binary (.dll/.so) to callers that
 * CANNOT be recompiled. Therefore the rules below are absolute:
 *
 *   1. Every public, caller-allocated struct begins with a `uint32_t
 *      struct_size` field. The caller sets it to sizeof(TheStruct) as it
 *      saw the struct at ITS compile time. The library uses that value to
 *      tell which fields are actually present, so old callers keep working
 *      against new libraries and vice versa.
 *
 *   2. Struct fields are ONLY ever appended at the end. Never reorder,
 *      never resize, never repurpose, never delete an existing field. A
 *      removed or moved field silently corrupts every existing binary.
 *
 *   2a. PADDING WARNING. When appending a field, make sure its offset is at
 *      or beyond the PREVIOUS version's sizeof — i.e. the new field must not
 *      fall inside trailing padding that an older struct already counted in
 *      its sizeof. If the previous struct ended with tail padding, insert an
 *      explicit reserved field (e.g. `uint32_t _reserved0;`) to consume it
 *      before adding the real field, so the struct_size check can tell the
 *      versions apart. (The bundled abi_test.c demonstrates the failure mode
 *      and the fix.) Prefer pointer-sized or grouped fields to minimize new
 *      padding.
 *
 *   3. The backend technology (Vulkan today) is NEVER named in a type the
 *      caller embeds by value. GPU devices live behind the opaque
 *      CevgGpuDevice handle, created by a backend-specific factory
 *      (cevg_gpu_device_create_vulkan). A future Metal/Dawn/WebGPU backend
 *      adds a NEW factory function and changes nothing here.
 *
 *   4. Enums only ever gain new values at the end. Existing values keep
 *      their integer meaning forever.
 *
 *   5. Functions are only ever added, never removed and never changed in
 *      signature. A behavioral variant gets a new name (…_ex, …_v2).
 *
 *   6. Arrays handed across the ABI are walked with a caller-supplied
 *      element stride (see cevg_text_blob_get_runs), so the element struct
 *      can grow without breaking array indexing in old binaries.
 *
 *   7. PLANNED EXTENSIONS. Lottie (Skottie) and SVG rendering are intended
 *      future features. They will be added strictly additively, as new
 *      opaque handles (CevgAnimation, CevgSvg) plus new cevg_animation_* /
 *      cevg_svg_* functions layered on CevgContext/CevgCanvas — nothing in
 *      this header changes for them. The ONE piece pulled forward is the
 *      CevgResourceProvider callback struct (defined below), because
 *      callback signatures are painful to change once hosts implement them;
 *      freezing its shape now lets the rest land later without an ABI break.
 *
 * Helpers for implementing rule 1 live just below.
 * ===================================================================== */

/* True if a caller-provided struct of type `type` (whose leading
 * struct_size the caller set) actually carries `field` — i.e. the caller
 * compiled against a header new enough to include it. Usage in the library:
 *     if (CEVG_HAS_FIELD(CevgConfig, cfg->struct_size, sample_count)) {...}
 * Written without compiler extensions (no __typeof__) so it works on MSVC,
 * and reproducible verbatim by bindings in other languages. */
#define CEVG_HAS_FIELD(type, given_size, field) \
    ((given_size) >= (uint32_t)(offsetof(type, field) + sizeof(((type*)0)->field)))

/* ---- Configuration ---- */

/* Opaque GPU device. The library does NOT create the underlying device —
 * the host creates it (sharing it with its own renderer if it likes) and
 * wraps it with a backend-specific factory below. The caller never learns
 * from this type whether the backend is Vulkan, Metal, or anything else,
 * which is exactly what lets the backend change without an ABI break. */
typedef struct CevgGpuDevice_ CevgGpuDevice;

#ifdef CEVG_VULKAN
/* Vulkan device parameters. This struct names Vulkan, but the caller does
 * NOT embed it in any long-lived type — it is consumed once, by the
 * factory, and may be stack-allocated and discarded immediately after.
 * That keeps the Vulkan vocabulary out of the permanent ABI surface
 * (CevgConfig holds a CevgGpuDevice*, never this).
 *
 * struct_size MUST be set to sizeof(CevgVulkanDevice) by the caller. */
typedef struct {
    uint32_t     struct_size;          /* = sizeof(CevgVulkanDevice) */
    void*        vk_instance;          /* VkInstance            */
    void*        vk_physical_device;   /* VkPhysicalDevice      */
    void*        vk_device;            /* VkDevice              */
    void*        vk_queue;             /* VkQueue (graphics)    */
    uint32_t     vk_queue_index;       /* graphics queue family */
    const char** enabled_ext_names;    /* device extensions the host enabled */
    uint32_t     enabled_ext_count;
    /* OPTIONAL: vkGetInstanceProcAddr. NULL -> library loads it from the
     * system Vulkan loader. Provide it for a custom loader (e.g. volk).
     * Typed as void* to keep vulkan.h out of this header. */
    void*        vk_get_instance_proc_addr;
    /* OPTIONAL: the Vulkan API version the host created its VkInstance with
     * (e.g. VK_API_VERSION_1_3, encoded as Vulkan does). 0 -> Cevg uses
     * VK_API_VERSION_1_1. Passing the real value lets Graphite opt into
     * features from newer Vulkan versions; never set it higher than the
     * version the host's instance/device actually support. */
    uint32_t     api_version;
    /* --- append future Vulkan-only fields here, never above --- */
} CevgVulkanDevice;

/* Wrap a host-created Vulkan device as a Cevg GPU device. The returned
 * handle owns no Vulkan objects (the host still owns/destroys the
 * VkInstance/VkDevice/etc.); it only records them for Cevg's use. Destroy
 * with cevg_gpu_device_destroy AFTER the context that used it is gone.
 * NULL on failure (e.g. missing required handles or struct_size too small).
 *
 * A future backend would add e.g. cevg_gpu_device_create_metal(...)
 * returning the same CevgGpuDevice* type — no change to anything else. */
CEVG_API CevgGpuDevice* cevg_gpu_device_create_vulkan(const CevgVulkanDevice* vk);
#endif /* CEVG_VULKAN */

/* Create a CPU-raster device (no GPU). A context created from it renders
 * entirely on the CPU through Skia's raster pipeline — use it when no usable
 * Vulkan device is available (headless, software-only, or a failed GPU init).
 * The typical pattern is: try cevg_gpu_device_create_vulkan first, and fall
 * back to this if it returns NULL:
 *
 *     CevgGpuDevice* dev = cevg_gpu_device_create_vulkan(&vk);
 *     if (!dev) dev = cevg_gpu_device_create_cpu();
 *
 * The CPU backend is a FIRST-CLASS peer of the Vulkan backend: identical
 * drawing/text/image output, offscreen surfaces, cevg_surface_read_pixels,
 * multithreaded recording (cevg_recorder_create / cevg_surface_create_with_recorder
 * / cevg_recorder_snap / cevg_context_insert_recording all work — draws simply
 * rasterize immediately on the worker thread and snap/insert are no-ops), and
 * window presentation. The ONE necessary difference is the presentation
 * mechanism: a software renderer cannot draw into a VkSurfaceKHR, so instead of
 * cevg_surface_create_for_vk_surface, the CPU backend presents into a host-owned
 * pixel buffer via cevg_surface_create_for_buffer + cevg_surface_present. Every
 * other entry point behaves identically across backends. Destroy with
 * cevg_gpu_device_destroy. */
CEVG_API CevgGpuDevice* cevg_gpu_device_create_cpu(void);

CEVG_API void           cevg_gpu_device_destroy(CevgGpuDevice* device);

/* Try Vulkan first; if unavailable, fall back to CPU. */
CEVG_API CevgGpuDevice* cevg_gpu_device_create_auto(void);

/* Context creation parameters. Caller-allocated; set struct_size to
 * sizeof(CevgConfig). Any field the caller's (older) struct_size does not
 * cover is treated as its documented default, so adding a field below is
 * always binary-compatible. */
typedef struct {
    uint32_t       struct_size;        /* = sizeof(CevgConfig) */
    CevgGpuDevice* device;             /* REQUIRED; NULL -> create fails */
    CevgColorSpace color_space;        /* default kCevgColorSpace_sRGB */
    int            sample_count;       /* MSAA hint, 0 = none */
    int            max_glyph_cache_size_kb; /* <=0 -> default 4096 */
    int            max_vram_cache_size_kb;  /* <=0 -> default 262144 (256 MiB) */
    /* --- append future config fields here, never above --- */
} CevgConfig;

/* Font metrics, in pixels. Sign convention matches SkFontMetrics:
 * ascent NEGATIVE, descent POSITIVE, others positive.
 *
 * This is an OUTPUT struct the caller allocates and the library fills.
 * The caller sets struct_size to its sizeof(CevgFontMetrics); the library
 * writes only the fields that fit, so an old caller's smaller struct is
 * never overrun when new metrics are appended. */
typedef struct {
    uint32_t struct_size;   /* = sizeof(CevgFontMetrics) */
    float ascent;
    float descent;
    float leading;
    float cap_height;
    float x_height;
    /* --- append future metrics here, never above --- */
} CevgFontMetrics;

/* A single BiDi run within a shaped TextBlob (visual order).
 *
 * This struct is returned in ARRAYS (cevg_text_blob_get_runs). To let it
 * grow without breaking array indexing in already-compiled callers, that
 * function takes the caller's element stride; see its declaration. There
 * is intentionally NO struct_size field here, because each element's size
 * is conveyed once via the stride parameter rather than per element. */
typedef struct {
    int               glyph_start;
    int               glyph_count;
    int               byte_start;
    int               byte_end;
    CevgTextDirection dir;
    /* --- append future per-run fields here, never above; callers pass
     *     sizeof(CevgTextRun) as the stride so growth stays compatible --- */
} CevgTextRun;

/* =====================================================================
 * Resource provider — for the planned Lottie (Skottie) and SVG features
 * ---------------------------------------------------------------------
 * Lottie/SVG content frequently references EXTERNAL resources by name:
 * images, fonts, and (for Lottie) nested animation blobs. Skia abstracts
 * this with skresources::ResourceProvider; this struct is the C, ABI-safe
 * equivalent, defined NOW so its shape is frozen before any host starts
 * implementing it (callbacks are the one thing that genuinely hurts to add
 * later — once a host implements them, the signatures can't move).
 *
 * The functions that CONSUME this struct (cevg_animation_*, cevg_svg_*) do
 * not exist yet; they will be added — purely additively — when the Lottie
 * and SVG features land. A CevgResourceProvider* will be an optional
 * argument to those future creation calls (NULL = no external resources;
 * embedded/data-URI resources still work). Defining the struct early costs
 * nothing and guarantees the eventual API drops in without an ABI break.
 *
 * DESIGN (deliberately minimal, zero Skia types crossing the boundary):
 *   - The host is asked only to RETURN BYTES for a named resource. Cevg
 *     turns those bytes into whatever Skia object is needed (decodes the
 *     image — including multi-frame — wraps the font as a typeface, parses
 *     a nested animation). The host never touches an SkImage/SkTypeface.
 *   - Every callback returns a pointer+length. BYTE LIFETIME: Cevg copies
 *     the returned bytes before the callback returns, so the host may free
 *     or reuse its buffer immediately. (Resource loads are infrequent, so
 *     the copy is irrelevant; this is by far the simplest contract for the
 *     host.) Return data=NULL / len=0 to indicate "not found / use the
 *     embedded resource".
 *   - The (path, name, id) triple mirrors Skia: `path` is a directory-ish
 *     prefix, `name` the file/resource name, `id` an asset identifier
 *     Lottie uses to refer back to the asset. Most hosts just concatenate
 *     path+name into a lookup key and ignore id; all three may be empty
 *     strings (never NULL).
 *
 * Caller-allocated and versioned like the other structs: set struct_size
 * to sizeof(CevgResourceProvider). New callbacks are appended at the end;
 * an older host's smaller struct_size simply means Cevg treats the missing
 * callbacks as absent and falls back to embedded resources.
 * ===================================================================== */
typedef struct {
    uint32_t struct_size;     /* = sizeof(CevgResourceProvider) */
    void*    user;            /* opaque; passed back to every callback */

    /* Load an image referenced by Lottie/SVG. Return the ENCODED image
     * bytes (PNG/JPEG/WebP/…); Cevg decodes them. data=NULL -> not found. */
    const void* (*load_image)(void* user, const char* path, const char* name,
                              const char* id, size_t* out_len);

    /* Load a font referenced by name/url. Return the font file bytes
     * (TTF/OTF/WOFF…); Cevg wraps them as a typeface. data=NULL -> fall
     * back to the system font manager. */
    const void* (*load_font)(void* user, const char* name, const char* url,
                             size_t* out_len);

    /* Load a generic resource — currently nested Lottie animations —
     * specified by path+name. Return the raw bytes. data=NULL -> not found. */
    const void* (*load_resource)(void* user, const char* path, const char* name,
                                 size_t* out_len);

    /* --- append future resource callbacks here, never above --- */
} CevgResourceProvider;

/* ---- Version / Backend ---- */

CEVG_API const char* cevg_version(void);
/* Always returns "skia_graphite_vulkan" in this build. Diagnostics only. */
CEVG_API const char* cevg_backend_name(void);

/* ---- Context ----
 * Owns the Graphite Context, the shared caches, and the font manager.
 * Threading: see cevg_recorder_* below. The Context itself (submit,
 * present, resource cleanup) must be touched only from the thread that
 * created it ("the GPU thread"). Recording can be parallelized via
 * additional recorders. */

CEVG_API CevgContext* cevg_context_create(const CevgConfig* config);
CEVG_API void         cevg_context_destroy(CevgContext* ctx);

/* Rebuild GPU resources after a device/swapchain loss (returns
 * kCevgErrorDeviceLost from present/flush). Recompiles pipelines and
 * recreates atlases. Surfaces and swapchains must be recreated by the
 * caller afterward. */
CEVG_API CevgResult   cevg_context_reset(CevgContext* ctx);

/* Run deferred cleanup: free GPU resources whose work has completed and
 * trim caches to budget. Call once per frame after present for steady
 * memory. NULL-safe. */
CEVG_API void         cevg_context_tick(CevgContext* ctx);

/* Current GPU memory used by Cevg's caches, in bytes (0 if NULL). */
CEVG_API size_t       cevg_context_get_vram_used(const CevgContext* ctx);
CEVG_API size_t       cevg_context_get_vram_budget(const CevgContext* ctx);

/* ---- Recorder (NEW) ----
 * Graphite records draw commands into a Recorder, then snaps a Recording
 * that the Context submits. Each CevgCanvas is bound to one recorder. The
 * context owns a default recorder (used when a canvas is created without
 * an explicit one). For multithreaded recording, create one recorder per
 * worker thread; recorders are independent and need no locking between
 * them. Only cevg_context submit/present touches the GPU queue and must
 * stay on the GPU thread. */
typedef struct CevgRecorder_ CevgRecorder;

CEVG_API CevgRecorder* cevg_recorder_create(CevgContext* ctx);
CEVG_API void          cevg_recorder_destroy(CevgRecorder* rec);
/* The context's built-in recorder (never destroy this one). */
CEVG_API CevgRecorder* cevg_context_default_recorder(CevgContext* ctx);

/* ---- Recorder snap / insert (NEW) — race-free multithreaded recording ----
 * A worker thread records into its OWN recorder, then snaps an immutable
 * CevgRecording ON THAT SAME THREAD. Snapping a recorder is not thread-safe, so
 * it must happen on the recording thread; the resulting CevgRecording, however,
 * is immutable and may be handed to the GPU/context thread, which inserts it and
 * then presents/flushes. This is the supported alternative to letting the
 * present path reach into a foreign thread's recorder.
 *
 * Snapping a recorder via cevg_recorder_snap removes it from the context's
 * automatic present-time snap set, so a recorder you snap explicitly is yours to
 * insert and will not be double-snapped by cevg_surface_present /
 * cevg_canvas_flush. (A recorder is re-added to that set whenever a new canvas is
 * created on one of its surfaces, so the per-frame cycle is: create canvas →
 * draw → snap.) */

/* Snap the recorder's pending draws into an immutable recording. Call on the
 * thread that recorded. Returns NULL if nothing was recorded or on failure. */
CEVG_API CevgRecording* cevg_recorder_snap(CevgRecorder* rec);

/* Queue a snapped recording into the context's command stream. Does NOT submit;
 * the next present/flush/read_pixels submits it. Call on the GPU/context thread.
 * Insert PRODUCER recordings (e.g. offscreen tiles) before the consumer that
 * samples them. Returns kCevgSuccess or kCevgErrorInvalidArg. The recording may
 * be destroyed any time after this returns. */
CEVG_API CevgResult    cevg_context_insert_recording(CevgContext* ctx,
                                                        CevgRecording* recording);

/* Destroy a recording (whether or not it was inserted). NULL-safe. */
CEVG_API void          cevg_recording_destroy(CevgRecording* recording);

/* ---- Surface ----
 * An RGBA_8888 / premultiplied / sRGB render target. */

/* Offscreen surface. If `pixels` is non-NULL it provides initial top-down
 * RGBA_8888 contents (stride w*4). NULL on failure. */
CEVG_API CevgSurface* cevg_surface_create(CevgContext* ctx, int w, int h, const void* pixels);

/* Offscreen surface bound to a specific recorder. This enables genuine
 * multithreaded recording: a worker thread creates its own CevgRecorder,
 * makes a surface from it here, and draws into it without touching the
 * default recorder or any other thread's recorder. The GPU thread then
 * composites the snapshot. NULL on failure. */
CEVG_API CevgSurface* cevg_surface_create_with_recorder(CevgRecorder* rec,
                                                            int w, int h,
                                                            const void* pixels);

#ifdef CEVG_VULKAN
/* Window surface with a real Vulkan swapchain for GPU-direct present.
 * `vk_surface` is a VkSurfaceKHR (as void*) the host already created from
 * its window (e.g. via SDL_Vulkan_CreateSurface / glfwCreateWindowSurface)
 * — Cevg stays windowing-system agnostic. `present_mode` selects vsync
 * behavior. After drawing, call cevg_surface_present(). NULL on failure.
 *
 * NOTE: this REPLACES the old create_for_window + set_present_data +
 * CPU-readback path. There is no CPU round-trip; the rendered image is
 * blitted/presented on the GPU. */
CEVG_API CevgSurface* cevg_surface_create_for_vk_surface(
    CevgContext* ctx, void* vk_surface, int w, int h, CevgPresentMode present_mode);
#endif /* CEVG_VULKAN */

/* Create a presentable surface backed by a host-owned CPU pixel buffer
 * (RGBA8888, premultiplied). This is the CPU backend's window-surface
 * equivalent: Cevg renders directly into `pixels` (zero copy), and after
 * drawing you call cevg_surface_present() (a flush) and then show/blit the
 * buffer to your window however you like — the standard software-present
 * model. The host owns `pixels` and must keep it alive and unchanged for the
 * surface's lifetime; use a second buffer for the next frame to avoid tearing.
 * `stride_bytes` is the row stride (0 = tightly packed, w*4).
 *
 * CPU backend only (returns NULL on a GPU context — there, present through a
 * Vulkan swapchain with cevg_surface_create_for_vk_surface). This is the
 * counterpart that gives the CPU backend full window-presentation parity. */
CEVG_API CevgSurface* cevg_surface_create_for_buffer(
    CevgContext* ctx, void* pixels, int w, int h, int stride_bytes);

/* Recreate a window surface's swapchain at a new size (after a window
 * resize). Keeps the same VkSurfaceKHR. Returns kCevgErrorDeviceLost if
 * the device is gone. No-op for offscreen surfaces. */
CEVG_API CevgResult   cevg_surface_resize(CevgSurface* surface, int w, int h);

CEVG_API void         cevg_surface_destroy(CevgSurface* surface);
CEVG_API int          cevg_surface_get_width(const CevgSurface* surface);
CEVG_API int          cevg_surface_get_height(const CevgSurface* surface);

/* Read the surface back to CPU memory: top-down RGBA_8888 premultiplied,
 * stride w*4, at least h*w*4 bytes. Finishes pending GPU work first.
 * Mostly for tests / screenshots — the present path does NOT use this. */
CEVG_API CevgResult   cevg_surface_read_pixels(CevgSurface* surface, void* out_pixels);

/* Present a window surface (swapchain acquire is handled internally at
 * canvas-create time; this snaps the recording, submits, and queues the
 * present). Non-blocking on the CPU (GPU-side semaphores order the work).
 * Returns kCevgSuccess, kCevgErrorDeviceLost (recreate swapchain), or
 * kCevgErrorUnsupported (offscreen surface). */
CEVG_API CevgResult   cevg_surface_present(CevgSurface* surface);

/* Mark a device-space rectangle dirty. When at least one dirty rect is
 * set before present, the backend may restrict the swapchain blit to the
 * union of dirty rects (partial present). With none set, the whole
 * surface is presented. Cleared automatically after each present. */
CEVG_API void         cevg_surface_add_dirty_rect(CevgSurface* surface,
                                                     int x, int y, int w, int h);

/* ---- Canvas ----
 * Records drawing into a surface via a recorder. Borrows the surface. */

/* Create a canvas on `surface` using the context's default recorder. For
 * a window surface this also acquires the next swapchain image. NULL on
 * failure (including swapchain out-of-date — recreate and retry). */
CEVG_API CevgCanvas*  cevg_canvas_create(CevgSurface* surface);

/* Create a canvas bound to an explicit recorder (for worker-thread
 * recording). The recorder and surface must share the same context. */
CEVG_API CevgCanvas*  cevg_canvas_create_with_recorder(CevgSurface* surface,
                                                          CevgRecorder* rec);

CEVG_API void         cevg_canvas_destroy(CevgCanvas* canvas);

CEVG_API void         cevg_canvas_save(CevgCanvas* canvas);
CEVG_API void         cevg_canvas_restore(CevgCanvas* canvas);
CEVG_API int          cevg_canvas_get_save_count(const CevgCanvas* canvas);

CEVG_API void         cevg_canvas_translate(CevgCanvas* canvas, float dx, float dy);
CEVG_API void         cevg_canvas_scale(CevgCanvas* canvas, float sx, float sy);
CEVG_API void         cevg_canvas_rotate(CevgCanvas* canvas, float degrees);
CEVG_API void         cevg_canvas_skew(CevgCanvas* canvas, float kx, float ky);
CEVG_API void         cevg_canvas_concat(CevgCanvas* canvas, const float matrix[9]);
CEVG_API void         cevg_canvas_reset_matrix(CevgCanvas* canvas);
CEVG_API void         cevg_canvas_get_transform(const CevgCanvas* canvas, float matrix[9]);

CEVG_API void         cevg_canvas_clip_rect(CevgCanvas* canvas, float x, float y, float w, float h);
CEVG_API void         cevg_canvas_clip_path(CevgCanvas* canvas, const CevgPath* path);
CEVG_API void         cevg_canvas_get_clip_bounds(CevgCanvas* canvas, float rect[4]);

CEVG_API void         cevg_canvas_clear(CevgCanvas* canvas, float r, float g, float b, float a);

/* Offscreen layer. alpha multiplies the paint's alpha; bounds is optional
 * LTRB; paint supplies blend, image filter, and backdrop filter. */
CEVG_API void         cevg_canvas_save_layer(CevgCanvas* canvas, float alpha,
                                                const float bounds[4], CevgPaint* paint);
CEVG_API void         cevg_canvas_restore_layer(CevgCanvas* canvas);

CEVG_API void         cevg_canvas_draw_paint(CevgCanvas* canvas, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_rect(CevgCanvas* canvas, float x, float y, float w, float h, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_round_rect(CevgCanvas* canvas, float x, float y, float w, float h, float rx, float ry, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_circle(CevgCanvas* canvas, float cx, float cy, float r, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_oval(CevgCanvas* canvas, float cx, float cy, float rx, float ry, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_line(CevgCanvas* canvas, float x0, float y0, float x1, float y1, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_path(CevgCanvas* canvas, const CevgPath* path, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_image(CevgCanvas* canvas, const CevgImage* image, float x, float y, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_image_rect(CevgCanvas* canvas, const CevgImage* image, const float src_rect[4], const float dst_rect[4], CevgPaint* paint);
/* Draw a nine-patch image.  `center` is LTRB in image-pixel space
 * (left, top, right, bottom) defining the stretchable center region;
 * the four corners are drawn at their original pixel size, the four
 * edges are stretched along one axis, and the center is stretched
 * along both axes.  `dst` is XYWH in user space.  On the Graphite
 * backend this is decomposed into up to 9 drawImageRect calls
 * because SkCanvas::drawImageNine is not yet supported. */
CEVG_API void         cevg_canvas_draw_image_nine(CevgCanvas* canvas, const CevgImage* image, const float center[4], const float dst[4], CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_text_blob(CevgCanvas* canvas, const CevgTextBlob* blob, float x, float y, CevgPaint* paint);
CEVG_API void         cevg_canvas_draw_surface(CevgCanvas* canvas, const CevgSurface* surface, float x, float y, CevgPaint* paint);

/* Flush this canvas' recorder: snap a Recording and submit it WITHOUT
 * blocking the CPU and WITHOUT presenting. Rarely needed — present()
 * and read_pixels() flush as required. */
CEVG_API void         cevg_canvas_flush(CevgCanvas* canvas);

/* ---- Display list (NEW) ----
 * Record a reusable sequence of draw commands once, then replay it many
 * times — optionally with a different transform each replay — without
 * re-running the cevg_canvas_draw_* calls. This is what makes scrolling
 * and transform animation cheap: the heavy recording happens once; each
 * frame only re-issues it with a new matrix. */

/* Begin capturing draws into a display list of the given size. Returns a
 * canvas whose draws are recorded (not rasterized) until record_end.
 * Coordinates are in the display list's own space. NULL on failure. */
CEVG_API CevgCanvas*      cevg_display_list_record_begin(CevgContext* ctx, int w, int h);
/* Finish capture; returns the immutable display list (destroy with
 * cevg_display_list_destroy). The capture canvas is consumed. */
CEVG_API CevgDisplayList* cevg_display_list_record_end(CevgCanvas* capture_canvas);
CEVG_API void             cevg_display_list_destroy(CevgDisplayList* dl);

/* Replay a display list onto `canvas`. `matrix` (column-major 9-float,
 * may be NULL for identity) is concatenated with the canvas CTM, so the
 * same list can be drawn at many positions/scales. The canvas clip
 * applies. */
CEVG_API void             cevg_canvas_draw_display_list(CevgCanvas* canvas,
                                                           const CevgDisplayList* dl,
                                                           const float matrix[9]);

/* ---- Path (backend-agnostic, cevg_common.c) ---- */
CEVG_API CevgPath*    cevg_path_create(void);
CEVG_API void         cevg_path_destroy(CevgPath* path);
CEVG_API void         cevg_path_rewind(CevgPath* path);
CEVG_API void         cevg_path_move_to(CevgPath* path, float x, float y);
CEVG_API void         cevg_path_line_to(CevgPath* path, float x, float y);
CEVG_API void         cevg_path_quad_to(CevgPath* path, float cx, float cy, float x, float y);
CEVG_API void         cevg_path_cubic_to(CevgPath* path, float c1x, float c1y, float c2x, float c2y, float x, float y);
CEVG_API void         cevg_path_close(CevgPath* path);
CEVG_API void         cevg_path_set_fill_rule(CevgPath* path, CevgFillRule rule);
CEVG_API CevgFillRule cevg_path_get_fill_rule(const CevgPath* path);
CEVG_API void         cevg_path_get_bounds(const CevgPath* path, float rect[4]);

/* ---- Paint (backend-agnostic, cevg_common.c) ----
 * Defaults: color = opaque black, style = Fill, alpha = 1, anti_alias =
 * true, blend = SrcOver, stroke_width = 1, cap = Butt, join = Miter,
 * miter = 4, filter_quality = Linear. Every setter bumps a generation
 * counter the backend uses to cache the resolved SkPaint. */
CEVG_API CevgPaint*   cevg_paint_create(void);
CEVG_API void         cevg_paint_destroy(CevgPaint* paint);
CEVG_API void         cevg_paint_reset(CevgPaint* paint);
CEVG_API void         cevg_paint_set_color(CevgPaint* paint, float r, float g, float b, float a);
CEVG_API void         cevg_paint_set_color_argb(CevgPaint* paint, uint32_t argb);
CEVG_API void         cevg_paint_set_style(CevgPaint* paint, CevgPaintStyle style);
CEVG_API void         cevg_paint_set_alpha(CevgPaint* paint, float alpha);
CEVG_API void         cevg_paint_set_anti_alias(CevgPaint* paint, bool enabled);
CEVG_API void         cevg_paint_set_blend_mode(CevgPaint* paint, CevgBlendMode mode);
CEVG_API void         cevg_paint_set_stroke_width(CevgPaint* paint, float width);
CEVG_API void         cevg_paint_set_stroke_cap(CevgPaint* paint, CevgCap cap);
CEVG_API void         cevg_paint_set_stroke_join(CevgPaint* paint, CevgJoin join);
CEVG_API void         cevg_paint_set_stroke_miter(CevgPaint* paint, float limit);
CEVG_API void         cevg_paint_set_dash(CevgPaint* paint, const float* dashes, int count, float phase);
CEVG_API void         cevg_paint_clear_dash(CevgPaint* paint);
CEVG_API void         cevg_paint_set_linear_gradient(CevgPaint* paint, float x0, float y0, float x1, float y1, const uint32_t* colors, const float* stops, int count, CevgTileMode tile);
CEVG_API void         cevg_paint_set_radial_gradient(CevgPaint* paint, float cx, float cy, float radius, const uint32_t* colors, const float* stops, int count, CevgTileMode tile);
CEVG_API void         cevg_paint_clear_shader(CevgPaint* paint);
CEVG_API void         cevg_paint_set_image_shader(CevgPaint* paint, CevgImage* image, CevgTileMode tile_x, CevgTileMode tile_y);
CEVG_API void         cevg_paint_set_blur(CevgPaint* paint, float sigma_x, float sigma_y);
CEVG_API void         cevg_paint_set_drop_shadow(CevgPaint* paint, float dx, float dy, float sigma, uint32_t color);
CEVG_API void         cevg_paint_set_color_matrix(CevgPaint* paint, const float matrix[20]);
CEVG_API void         cevg_paint_set_filter_quality(CevgPaint* paint, CevgFilterQuality quality);
CEVG_API void         cevg_paint_clear_filter(CevgPaint* paint);
CEVG_API void         cevg_paint_set_backdrop_blur(CevgPaint* paint, float sigma_x, float sigma_y);
CEVG_API void         cevg_paint_set_backdrop_shadow(CevgPaint* paint, float dx, float dy, float sigma, uint32_t color);
CEVG_API void         cevg_paint_clear_backdrop(CevgPaint* paint);

/* ---- Typeface (reference-counted) ---- */
CEVG_API CevgTypeface* cevg_typeface_create_from_file(const char* path);
CEVG_API CevgTypeface* cevg_typeface_create_from_data(const void* data, size_t len);
CEVG_API void          cevg_typeface_ref(CevgTypeface* typeface);
CEVG_API void          cevg_typeface_unref(CevgTypeface* typeface);
CEVG_API void          cevg_typeface_get_metrics(const CevgTypeface* typeface, float font_size, CevgFontMetrics* metrics);

/* ---- TextBlob (immutable shaped run) ---- */
CEVG_API CevgTextBlob* cevg_text_blob_make(const char* text, size_t len, const CevgTypeface* typeface, float size, CevgTextDirection dir);
CEVG_API CevgTextBlob* cevg_text_blob_make_ex(const char* text, size_t len, const CevgTypeface* typeface, float size, CevgTextDirection dir, const CevgTypeface** fallbacks, int fallback_count);
CEVG_API void          cevg_text_blob_destroy(CevgTextBlob* blob);
CEVG_API float         cevg_text_blob_get_width(const CevgTextBlob* blob);
CEVG_API float         cevg_text_blob_get_height(const CevgTextBlob* blob);
CEVG_API int           cevg_text_blob_get_glyph_count(const CevgTextBlob* blob);
CEVG_API void          cevg_text_blob_get_glyph_positions(const CevgTextBlob* blob, float* out_x, float* out_y);
CEVG_API void          cevg_text_blob_get_cluster_info(const CevgTextBlob* blob, int* char_indices);
CEVG_API int           cevg_text_blob_hit_test(const CevgTextBlob* blob, float x, float y);
CEVG_API void          cevg_text_blob_get_glyph_advances(const CevgTextBlob* blob, float* out_advances);
CEVG_API int           cevg_text_blob_get_run_count(const CevgTextBlob* blob);
/* Fill an array of run records. `out_runs` points to `count` records each
 * `run_stride` bytes apart; pass count = cevg_text_blob_get_run_count() and
 * run_stride = sizeof(CevgTextRun) as the caller sees it. Writing by the
 * caller's stride is what lets CevgTextRun gain fields later without
 * corrupting array traversal in already-compiled binaries. The library
 * writes only the fields that fit in run_stride. */
CEVG_API void          cevg_text_blob_get_runs(const CevgTextBlob* blob,
                                                  CevgTextRun* out_runs,
                                                  int count, size_t run_stride);
CEVG_API int           cevg_text_find_line_breaks(const char* text, size_t len, int* out_breaks, int max_breaks);

/* ---- Image (RGBA_8888, non-premultiplied source) ---- */
CEVG_API CevgResult    cevg_image_create_from_file(const char* path, CevgImage** out_image);
CEVG_API CevgResult    cevg_image_create_from_memory(const void* data, size_t len, CevgImage** out_image);
CEVG_API CevgImage*    cevg_image_create_from_pixels(const void* pixels, int w, int h);
CEVG_API void          cevg_image_destroy(CevgImage* image);
CEVG_API int           cevg_image_get_width(const CevgImage* image);
CEVG_API int           cevg_image_get_height(const CevgImage* image);

#ifdef CEVG_VULKAN
/* ---- GPU Profiler ----
 * Lightweight GPU timing via Vulkan timestamp queries.  Usage:
 *   1. cevg_gpu_profiler_enable(ctx, true)  — enable once at startup
 *   2. cevg_gpu_profiler_begin_frame(ctx)   — start of frame
 *   3. cevg_gpu_profiler_end_frame(ctx)     — end of frame (after present)
 *   4. cevg_gpu_profiler_get_frame_time(ctx) — retrieve last completed frame's GPU time (ms)
 *
 * The profiler uses a ring of VkQueryPool timestamp queries (8 frames deep).
 * Results are available one frame after they are recorded.  Overhead when
 * disabled: zero (all functions early-return). */

CEVG_API void    cevg_gpu_profiler_enable(CevgContext* ctx, bool enable);
CEVG_API void    cevg_gpu_profiler_begin_frame(CevgContext* ctx);
CEVG_API void    cevg_gpu_profiler_end_frame(CevgContext* ctx);
CEVG_API float   cevg_gpu_profiler_get_frame_time(const CevgContext* ctx);  /* ms, 0 if no data */
#endif /* CEVG_VULKAN */

#ifdef __cplusplus
}
#endif
#endif /* CEVG_H */
