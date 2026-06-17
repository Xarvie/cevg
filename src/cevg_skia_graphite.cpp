/* =====================================================================
 * cevg_skia_graphite.cpp — Cevg VG backend (Skia Graphite + Vulkan)
 * ---------------------------------------------------------------------
 * Single backend. Implements every cevg_* entry point that is not Path
 * or Paint (those live in cevg_common.c). Design goals, in priority
 * order, all of which fix problems in the previous backend:
 *
 *   1. GPU-direct presentation. A window surface owns a real Vulkan
 *      swapchain. Present = snap a Recording, submit it with
 *      SyncToCpu::kNo, and blit into the acquired swapchain image, all
 *      ordered by GPU semaphores. No asyncRescaleAndReadPixels, no
 *      SDL_Texture upload, no per-frame CPU stall. (Previous backend did
 *      a full-frame readback + CPU upload every frame.)
 *
 *   2. Resolved-paint caching keyed on CevgPaint::generation, so a paint
 *      that does not change between draws is converted to SkPaint (with
 *      its shader/colorfilter/imagefilter) exactly once.
 *
 *   3. Multi-recorder ready. The Context owns one default Recorder, but
 *      callers can create more (one per worker thread) and bind a canvas
 *      to a specific recorder. Only submit/present touch the GPU queue.
 *
 *   4. Retained display lists via SkPicture, replayable with a per-call
 *      matrix — cheap scrolling and transform animation.
 *
 *   5. Portable font manager (FreeType/Fontconfig where available,
 *      CoreText on Apple, DirectWrite on Windows) selected at build time
 *      — not hardcoded to DirectWrite.
 *
 * The Vulkan<->Graphite swapchain glue is the only part that depends on
 * Skia-checkout-private headers; it is isolated in the CevgSwapchain
 * section and marked. Everything else uses the stable public Skia API.
 * ===================================================================== */

#include "cevg_internal.h"

/* ---- Vulkan ---- */
#ifdef CEVG_VULKAN
#include <glad/vulkan.h>
#endif

/* ---- Skia core (stable public API) ---- */
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImage.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkShader.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkStream.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkM44.h"
#include "include/core/SkClipOp.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkGraphics.h"

#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradient.h"
#include "include/effects/SkImageFilters.h"
#include "include/effects/SkColorMatrixFilter.h"

#include "include/encode/SkPngEncoder.h"
#include "include/codec/SkCodec.h"

/* ---- Skia shaping ---- */
#include "modules/skshaper/include/SkShaper.h"
#include "modules/skshaper/include/SkShaper_harfbuzz.h"

/* ---- Skia Unicode (UAX #14 line breaking via internal ICU) ---- */
#include "modules/skunicode/include/SkUnicode.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

/* ---- Skia Graphite (stable public API) ---- */
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/GraphiteTypes.h"
#include "include/gpu/graphite/Image.h"
#include "include/gpu/graphite/Recorder.h"
#include "include/gpu/graphite/Recording.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/BackendSemaphore.h"
#include "include/gpu/graphite/TextureInfo.h"
#ifdef CEVG_VULKAN
#include "include/gpu/graphite/vk/VulkanGraphiteContext.h"
#include "include/gpu/graphite/vk/VulkanGraphiteTypes.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/vk/VulkanMemoryAllocator.h"
#include "include/gpu/vk/VulkanExtensions.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"
#include "src/gpu/vk/vulkanmemoryallocator/VulkanMemoryAllocatorPriv.h"
#endif
#include "src/gpu/GpuTypesPriv.h"
#include "include/gpu/MutableTextureState.h"

/* ---- Platform font managers (with full fallback support) ---- */
#if defined(SK_BUILD_FOR_WIN)
#include "include/ports/SkTypeface_win.h"
#elif defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_IOS)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(SK_BUILD_FOR_ANDROID)
#include "include/ports/SkFontMgr_android_ndk.h"
#elif defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif
/* Fallback: FreeType Custom Directory (embedded / no platform FontMgr) */
#include "include/ports/SkFontMgr_directory.h"
#include "include/ports/SkFontMgr_empty.h"

#include <cstring>
#include <cstdio>
#include <new>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace graphite = skgpu::graphite;

/* Global flag: set when cevg_gpu_device_create_cpu() is called. */
static bool g_cevg_cpu_mode = false;

/* ===================================================================
 * Forward declarations
 * -------------------------------------------------------------------
 * Every static helper that is referenced before its definition must be
 * declared here. C++ has no implicit function declarations, so a use that
 * precedes the definition is a hard compile error without these. (The
 * previous revision called cevg_context_snap_and_submit, _register_recorder
 * and cevg_shared_fontmgr ahead of their bodies and would not build.)
 *
 * graphite::SyncToCpu and graphite::Recorder are already fully defined here
 * (GraphiteTypes.h / Recorder.h are included above), so we use the real
 * types rather than risk a mismatched enum forward declaration.
 * =================================================================== */
#ifdef CEVG_VULKAN
struct CevgSwapchain;  /* defined in the swapchain section */
#endif

static sk_sp<SkImage>   EnsureGraphiteImage(const CevgImage* image, CevgContext* ctx);
static sk_sp<SkImage>   EnsureGraphiteImageRec(const CevgImage* image, graphite::Recorder* rec);
static sk_sp<SkFontMgr> cevg_make_fontmgr();
static sk_sp<SkFontMgr> cevg_shared_fontmgr();
static void             cevg_context_register_recorder(CevgContext* ctx,
                                                       graphite::Recorder* rec);
static void             cevg_context_snap_and_insert(CevgContext* ctx);
static void             cevg_context_snap_and_submit(CevgContext* ctx,
                                                     graphite::SyncToCpu sync);

/* ===================================================================
 * Context / Recorder
 * =================================================================== */

/* Opaque GPU device. Today it carries the host's Vulkan handles, or marks a
 * CPU-raster fallback context; a future Metal/Dawn device can share the type
 * and cevg_context_create branches on `backend`. The device owns no Vulkan
 * objects — the host created them and destroys them. */
enum class CevgGpuBackend { Vulkan, CPU };

struct CevgGpuDevice_ {
    CevgGpuBackend backend;
#ifdef CEVG_VULKAN
    /* Vulkan handles (valid when backend == Vulkan). */
    VkInstance       instance;
    VkPhysicalDevice phys;
    VkDevice         device;
    VkQueue          queue;
    uint32_t         queueFamily;
    PFN_vkGetInstanceProcAddr getInstanceProc;
    uint32_t         apiVersion;          /* host's instance API version; 0 -> default 1.1 */
    std::vector<const char*> enabledExt;  /* copied; caller's array may be transient */
#endif
};

#ifdef CEVG_VULKAN
/* ---- Vulkan dispatch table ----
 * Function pointers for every Vulkan command Cevg calls directly (i.e. outside
 * Graphite, which loads its own). Populated once in cevg_context_create from the
 * host's vkGetInstanceProcAddr, so Cevg never references the global Vulkan
 * symbols and works with a custom / volk-style loader that exports nothing.
 * Member names are the command name minus the "vk" prefix. */
struct CevgVkApi {
    /* instance-level (loaded via vkGetInstanceProcAddr) */
    PFN_vkEnumerateInstanceExtensionProperties     EnumerateInstanceExtensionProperties;
    PFN_vkEnumerateDeviceExtensionProperties       EnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceProperties              GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties        GetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR  GetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR       GetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR  GetPhysicalDeviceSurfacePresentModesKHR;
    /* device-level (loaded via vkGetDeviceProcAddr) */
    PFN_vkDeviceWaitIdle             DeviceWaitIdle;
    PFN_vkCreateSwapchainKHR         CreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR        DestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR      GetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR        AcquireNextImageKHR;
    PFN_vkQueuePresentKHR            QueuePresentKHR;
    PFN_vkCreateImage                CreateImage;
    PFN_vkDestroyImage               DestroyImage;
    PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
    PFN_vkAllocateMemory             AllocateMemory;
    PFN_vkFreeMemory                 FreeMemory;
    PFN_vkBindImageMemory            BindImageMemory;
    PFN_vkCreateSemaphore            CreateSemaphore;
    PFN_vkDestroySemaphore           DestroySemaphore;
    PFN_vkCreateFence                CreateFence;
    PFN_vkDestroyFence               DestroyFence;
    PFN_vkWaitForFences              WaitForFences;
    PFN_vkResetFences                ResetFences;
    PFN_vkCreateCommandPool          CreateCommandPool;
    PFN_vkDestroyCommandPool         DestroyCommandPool;
    PFN_vkAllocateCommandBuffers     AllocateCommandBuffers;
    PFN_vkResetCommandBuffer         ResetCommandBuffer;
    PFN_vkBeginCommandBuffer         BeginCommandBuffer;
    PFN_vkEndCommandBuffer           EndCommandBuffer;
    PFN_vkQueueSubmit                QueueSubmit;
    PFN_vkCmdPipelineBarrier         CmdPipelineBarrier;
    PFN_vkCmdCopyImage               CmdCopyImage;
    PFN_vkCmdBlitImage               CmdBlitImage;
    PFN_vkCmdWriteTimestamp          CmdWriteTimestamp;
    PFN_vkCreateQueryPool            CreateQueryPool;
    PFN_vkDestroyQueryPool           DestroyQueryPool;
    PFN_vkResetQueryPool             ResetQueryPool;
    PFN_vkGetQueryPoolResults        GetQueryPoolResults;
};
#endif

struct CevgContext_ {
    std::unique_ptr<graphite::Context>  ctx;
    /* Default recorder; canvases without an explicit recorder use it. */
    std::unique_ptr<graphite::Recorder> recorder;

    bool isCpu = false;                 /* true => CPU raster fallback (no Graphite/Vulkan) */

    sk_sp<SkFontMgr> fontMgr;
    sk_sp<SkColorSpace> colorSpace;     /* working color space (sRGB or linear) */

#ifdef CEVG_VULKAN
    skgpu::VulkanExtensions vkExtensions;

    /* Cached Vulkan handles (needed by the swapchain code). */
    VkInstance       instance;
    VkPhysicalDevice phys;
    VkDevice         device;
    VkQueue          queue;
    uint32_t         queueFamily;
    PFN_vkGetInstanceProcAddr getInstanceProc;
    CevgVkApi        vk;             /* resolved Vulkan entry points (see cevg_vk_api_load) */
#endif

    CevgColorSpace cevg_cs;
    int            sample_count;
    int            glyph_cache_kb;
    size_t         vram_budget;

    /* Active recorders: every recorder that was bound to a canvas this
     * frame.  cevg_surface_present / cevg_canvas_flush snap ALL of them
     * so that multi-recorder recording does not lose draws.  Cleared
     * after each snap. */
    std::vector<graphite::Recorder*> active_recorders;
    std::mutex                       active_recorders_mutex;

    /* Cached wrapper for the context's default recorder (borrowed, not
     * owned).  Created lazily by cevg_context_default_recorder, freed
     * in cevg_context_destroy.  Avoids the old global-static cache that
     * leaked entries after context destruction. */
    CevgRecorder* default_recorder_wrapper;

    /* GPU profiler state. */
    bool           profilerEnabled;
#ifdef CEVG_VULKAN
    VkQueryPool    profilerQueryPool;     /* 2 queries per frame: begin + end */
#endif
    static constexpr int kProfilerRingSize = 8;
    int            profilerRingIndex;     /* current write slot */
    uint64_t       profilerTimestamps[kProfilerRingSize * 2]; /* cached results */
    bool           profilerHasData;
    float          profilerLastMs;        /* GPU time of the most recent completed frame */
    float          timestampPeriod;       /* nanoseconds per timestamp tick */
};

struct CevgRecorder_ {
    std::unique_ptr<graphite::Recorder> owned;  /* set when this recorder owns its graphite::Recorder */
    graphite::Recorder* borrowed;               /* set for the context default recorder (not owned) */
    CevgContext* ctx;
    graphite::Recorder* get() const { return owned ? owned.get() : borrowed; }

    /* CPU backend: the worker's offscreen surface. When the worker draws on its
     * own thread, it rasterizes directly into this surface. On snap(), a
     * snapshot (SkImage) is taken and stored in the CevgRecording so that
     * insert_recording() can blit it onto the main surface. */
    sk_sp<SkSurface> cpuSurface;
    int cpuW = 0, cpuH = 0;
};

/* ===================================================================
 * Surface / Canvas
 * =================================================================== */
struct CevgDirtyRect { int x, y, w, h; };

struct CevgSurface_ {
    sk_sp<SkSurface> surface;    /* the render target the canvas draws into */
    CevgContext* ctx;
    int width;
    int height;

    /* The recorder that OWNS this surface. In Graphite every SkSurface is
     * created from a specific Recorder and can only be drawn into through
     * that recorder, so a canvas on this surface records into exactly this
     * recorder — never a foreign one. For an offscreen surface this is the
     * recorder passed to cevg_surface_create_with_recorder (or the context
     * default). For a window surface it is always the context default,
     * because the swapchain's render target is owned by it.
     *
     * This is the field that makes real multi-recorder work: a worker
     * thread creates its own CevgRecorder, an offscreen surface from it,
     * draws in parallel, and the GPU thread composites the snapshot. */
    graphite::Recorder* ownerRecorder;

    /* Window presentation: present via a real Vulkan swapchain. NULL for
     * offscreen surfaces. */
#ifdef CEVG_VULKAN
    CevgSwapchain* swapchain;
#else
    void*          swapchain;  /* always nullptr without Vulkan */
#endif

    /* CPU backend: true if this surface renders directly into a host-owned
     * pixel buffer (cevg_surface_create_for_buffer) and is therefore
     * presentable. False for CPU offscreen surfaces (read_pixels only). */
    bool cpuPresentable = false;

    std::vector<CevgDirtyRect> dirty;
};

struct CevgCanvas_ {
    SkCanvas*    canvas;     /* borrowed from surface->surface or a picture recorder */
    CevgSurface* surface;    /* NULL for display-list capture canvases */
    CevgRecorder* recorder;  /* recorder this canvas records into (owns surface) */

    /* Display-list capture mode. */
    SkPictureRecorder* picRec;  /* non-NULL while capturing a display list */
    int dlWidth, dlHeight;      /* display list dimensions (for record_end) */
};

struct CevgDisplayList_ {
    sk_sp<SkPicture> picture;
    int width, height;
};

/* A snapped, immutable Graphite recording, produced by cevg_recorder_snap on
 * the recording thread and consumed by cevg_context_insert_recording on the
 * GPU thread. Holds the recording until inserted+submitted (then destroyed). */
struct CevgRecording_ {
    std::unique_ptr<graphite::Recording> recording;
    /* CPU backend: snapshot of the worker surface at snap() time.
     * insert_recording() blits this image onto the main surface. */
    sk_sp<SkImage> cpuSnapshot;
    int cpuW = 0, cpuH = 0;
};

/* ===================================================================
 * Typeface / TextBlob / Image
 * =================================================================== */
struct CevgTypeface_ {
    sk_sp<SkTypeface> typeface;
    std::atomic<int>  ref_count;   /* atomic: typefaces are shared across the
                                    * worker threads that build text blobs */
};

struct CevgTextBlob_ {
    sk_sp<SkTextBlob> native_blob;
    int glyph_count;
    uint16_t* glyph_ids;
    float*    positions_x;
    float*    positions_y;
    int*      cluster_indices;
    float     width;
    float     height;
    size_t    text_len;     /* source string byte length (for end-of-line cursor) */

    /* The primary typeface, ref'd for the blob's lifetime so it outlives the
     * blob even if the caller unrefs their handle. (Fallback fonts are already
     * baked into native_blob by the shaper, so no per-glyph face array is
     * needed.) */
    CevgTypeface*  primary_face;

    float*         advances;
    int            run_count;
    CevgTextRun*   runs;
    float          font_size;
    CevgTextDirection para_dir;
};

struct CevgImage_ {
    sk_sp<SkImage> image;       /* CPU-decoded source (immutable after create) */
    mutable sk_sp<SkImage> gpu_image;   /* cached Graphite-backed copy (write-once) */
    int width;
    int height;
    mutable std::mutex gpu_upload_mutex; /* guards the one-time gpu_image upload from
                                  * racing worker threads that draw the same image */
    /* Release/acquire gate for the lock-free fast path. gpu_image is written
     * exactly once (under the mutex) and then never mutated; gpu_ready is set
     * with release AFTER that write, and the fast-path read acquire-loads it.
     * This makes the unlocked read+copy of gpu_image safe — without it, the
     * fast path raced with the (non-atomic) sk_sp assignment under the lock. */
    mutable std::atomic<bool> gpu_ready{false};
    /* Reference count. Starts at 1 (the creator's handle, released by the
     * public cevg_image_destroy). A CevgPaint that installs this image as a
     * shader holds an additional reference, so destroying the caller's handle
     * while a paint still references the image is safe (no use-after-free). */
    std::atomic<int> ref_count{1};
};

/* ===================================================================
 * Enum conversions
 * =================================================================== */
static SkBlendMode ToSkBlend(CevgBlendMode mode) {
    static const SkBlendMode table[] = {
        SkBlendMode::kClear,      SkBlendMode::kSrc,       SkBlendMode::kDst,
        SkBlendMode::kSrcOver,    SkBlendMode::kDstOver,   SkBlendMode::kSrcIn,
        SkBlendMode::kDstIn,      SkBlendMode::kSrcOut,    SkBlendMode::kDstOut,
        SkBlendMode::kSrcATop,    SkBlendMode::kDstATop,   SkBlendMode::kXor,
        SkBlendMode::kPlus,       SkBlendMode::kModulate,  SkBlendMode::kScreen,
        SkBlendMode::kOverlay,    SkBlendMode::kDarken,    SkBlendMode::kLighten,
        SkBlendMode::kColorDodge, SkBlendMode::kColorBurn, SkBlendMode::kHardLight,
        SkBlendMode::kSoftLight,  SkBlendMode::kDifference,SkBlendMode::kExclusion,
        SkBlendMode::kMultiply,   SkBlendMode::kHue,       SkBlendMode::kSaturation,
        SkBlendMode::kColor,      SkBlendMode::kLuminosity,
    };
    static_assert(sizeof(table)/sizeof(table[0]) == 29, "blend mode count mismatch");
    if ((int)mode < 0 || (int)mode >= 29) return SkBlendMode::kSrcOver;
    return table[(int)mode];
}

static SkTileMode ToSkTile(CevgTileMode m) {
    switch (m) {
        case kCevgTile_Clamp:  return SkTileMode::kClamp;
        case kCevgTile_Repeat: return SkTileMode::kRepeat;
        case kCevgTile_Mirror: return SkTileMode::kMirror;
        default:               return SkTileMode::kClamp;
    }
}

static SkPaint::Cap ToSkCap(CevgCap c) {
    switch (c) {
        case kCevgCap_Butt:   return SkPaint::kButt_Cap;
        case kCevgCap_Round:  return SkPaint::kRound_Cap;
        case kCevgCap_Square: return SkPaint::kSquare_Cap;
        default:              return SkPaint::kButt_Cap;
    }
}

static SkPaint::Join ToSkJoin(CevgJoin j) {
    switch (j) {
        case kCevgJoin_Miter: return SkPaint::kMiter_Join;
        case kCevgJoin_Round: return SkPaint::kRound_Join;
        case kCevgJoin_Bevel: return SkPaint::kBevel_Join;
        default:              return SkPaint::kMiter_Join;
    }
}

static SkSamplingOptions ToSkSampling(CevgFilterQuality q) {
    switch (q) {
        case kCevgFilterQuality_Nearest: return SkSamplingOptions(SkFilterMode::kNearest);
        case kCevgFilterQuality_Linear:  return SkSamplingOptions(SkFilterMode::kLinear,
                                                                  SkMipmapMode::kLinear);
        case kCevgFilterQuality_High:
        default: return SkSamplingOptions(SkCubicResampler{1.0f/3.0f, 1.0f/3.0f});
    }
}

static SkColor ArgbToSkColor(uint32_t argb) {
    return SkColorSetARGB((argb >> 24) & 0xFF, (argb >> 16) & 0xFF,
                          (argb >> 8) & 0xFF, argb & 0xFF);
}

#ifdef CEVG_VULKAN
/* Map the swapchain's VkFormat to the matching SkColorType so the Cevg
 * render target is created with the SAME channel order as the swapchain
 * image. This is what makes the present-time image transfer a pure
 * passthrough: vkCmdBlitImage / vkCmdCopyImage copy by component position
 * and do NOT swizzle, so an R8G8B8A8 render image transferred into a
 * B8G8R8A8 swapchain image would swap red and blue. By matching formats we
 * avoid that entirely. Returns kUnknown_SkColorType for formats Skia can't
 * wrap, in which case the caller falls back to RGBA. */
static SkColorType VkFormatToSkColorType(VkFormat fmt) {
    switch (fmt) {
        case VK_FORMAT_R8G8B8A8_UNORM: return kRGBA_8888_SkColorType;
        case VK_FORMAT_B8G8R8A8_UNORM: return kBGRA_8888_SkColorType;
        default:                       return kUnknown_SkColorType;
    }
}
#endif

/* ===================================================================
 * Paint resolution (CevgPaint -> SkPaint)
 * -------------------------------------------------------------------
 * Converts into a per-THREAD scratch SkPaint. The result is consumed
 * synchronously by the canvas->draw* call that follows (Skia copies the
 * SkPaint into the recording immediately), so a thread-local scratch is
 * safe and needs no lifetime management.
 *
 * The previous design cached the resolved SkPaint ON the CevgPaint
 * (np->backend_cache), keyed on np->generation. That is unsafe under this
 * library's own threading model: a CevgPaint can be shared across worker
 * threads (e.g. one reused "black fill"), and two threads converting it at
 * once would (a) both allocate and race-write the cache pointer (leak +
 * torn pointer) and (b) one rebuild entry->paint while the other reads the
 * returned `const SkPaint&` — a use-after-write. We therefore rebuild per
 * call. Graphite still caches the heavy downstream artifacts (pipelines,
 * tessellated coverage, uploaded gradient/image data) keyed on the resolved
 * paint, so the per-draw cost is only the lightweight SkPaint assembly.
 *
 * If that assembly ever shows up in a profile, the right fix is a
 * per-recorder cache keyed on a STABLE paint id: add a `uint32_t uid` to
 * CevgPaint_ (mirroring CevgPath_) in cevg_internal.h, have cevg_common.c
 * assign it at create time, and memoize {uid,generation}->SkPaint inside
 * each CevgRecorder (never shared across threads). That keeps the cache
 * thread-private without the pointer-reuse hazard of keying on CevgPaint*.
 * =================================================================== */
static sk_sp<SkShader> BuildShader(CevgPaint* np, CevgContext* ctx) {
    if (np->has_linear_gradient && np->grad_count > 0) {
        std::vector<SkColor4f> colors(np->grad_count);
        std::vector<float> positions(np->grad_count);
        for (int i = 0; i < np->grad_count; i++) {
            colors[i] = SkColor4f::FromColor(ArgbToSkColor(np->grad_colors[i]));
            positions[i] = np->grad_stops ? np->grad_stops[i] : (np->grad_count > 1 ? (float)i / (np->grad_count - 1) : 0.0f);
        }
        SkPoint pts[2] = {
            {np->grad_pts[0], np->grad_pts[1]},
            {np->grad_pts[2], np->grad_pts[3]},
        };
        SkGradient::Colors gradColors(colors, positions, ToSkTile(np->grad_tile));
        SkGradient gradient(gradColors, {});
        return SkShaders::LinearGradient(pts, gradient);
    }
    if (np->has_radial_gradient && np->grad_count > 0) {
        std::vector<SkColor4f> colors(np->grad_count);
        std::vector<float> positions(np->grad_count);
        for (int i = 0; i < np->grad_count; i++) {
            colors[i] = SkColor4f::FromColor(ArgbToSkColor(np->grad_colors[i]));
            positions[i] = np->grad_stops ? np->grad_stops[i] : (np->grad_count > 1 ? (float)i / (np->grad_count - 1) : 0.0f);
        }
        SkPoint center = {np->grad_pts[0], np->grad_pts[1]};
        SkGradient::Colors gradColors(colors, positions, ToSkTile(np->grad_tile));
        SkGradient gradient(gradColors, {});
        return SkShaders::RadialGradient(center, np->grad_pts[2], gradient);
    }
    if (np->has_image_shader && np->image_shader) {
        sk_sp<SkImage> img = EnsureGraphiteImage(np->image_shader, ctx);
        if (img) {
            return img->makeShader(ToSkTile(np->image_tile_x),
                                   ToSkTile(np->image_tile_y),
                                   ToSkSampling(np->filter_quality));
        }
    }
    return nullptr;
}

static sk_sp<SkColorFilter> BuildColorFilter(CevgPaint* np) {
    if (np->has_color_matrix) return SkColorFilters::Matrix(np->color_matrix);
    return nullptr;
}

static sk_sp<SkImageFilter> BuildImageFilter(CevgPaint* np) {
    sk_sp<SkImageFilter> filter;
    if (np->has_blur) {
        filter = SkImageFilters::Blur(np->blur_sigma_x, np->blur_sigma_y, nullptr);
    }
    if (np->has_drop_shadow) {
        sk_sp<SkImageFilter> shadow = SkImageFilters::DropShadow(
            np->shadow_dx, np->shadow_dy, np->shadow_sigma, np->shadow_sigma,
            ArgbToSkColor(np->shadow_color), filter);
        filter = shadow;
    }
    return filter;
}

static const SkPaint& CevgPaintToSkPaint(CevgPaint* np, CevgContext* ctx) {
    /* Per-thread scratch; the caller (a canvas->draw* call) copies it into the
     * recording synchronously, so the returned reference only has to outlive
     * that immediate use on this same thread. */
    static thread_local SkPaint p;
    p = SkPaint();
    if (!np) {
        p.setColor(SK_ColorBLACK);
        p.setAntiAlias(true);
        return p;
    }

    p.setColor(SkColor4f{np->color[0], np->color[1], np->color[2],
                         np->color[3] * np->alpha});
    p.setStyle(np->style == kCevgStyle_Stroke ? SkPaint::kStroke_Style
                                              : SkPaint::kFill_Style);
    p.setAntiAlias(np->anti_alias);
    p.setBlendMode(ToSkBlend(np->blend_mode));
    p.setStrokeWidth(np->stroke_width);
    p.setStrokeCap(ToSkCap(np->stroke_cap));
    p.setStrokeJoin(ToSkJoin(np->stroke_join));
    p.setStrokeMiter(np->stroke_miter);

    if (np->dash_count > 0 && np->dash_pattern) {
        /* Their Skia build takes an SkSpan<const SkScalar>. dash_pattern is
         * float[]; SkScalar == float, so a span over it is valid. */
        p.setPathEffect(SkDashPathEffect::Make(
            SkSpan<const SkScalar>(np->dash_pattern, np->dash_count),
            np->dash_phase));
    }
    if (auto sh = BuildShader(np, ctx)) p.setShader(std::move(sh));
    if (auto cf = BuildColorFilter(np)) p.setColorFilter(std::move(cf));
    if (auto imf = BuildImageFilter(np)) p.setImageFilter(std::move(imf));
    return p;
}

static SkPath CevgPathToSkPath(const CevgPath* np) {
    SkPathBuilder b;
    b.setFillType(np->fill_rule == kCevgFillRule_EvenOdd
                      ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding);
    int ci = 0;
    for (int i = 0; i < np->count; i++) {
        switch (np->cmds[i]) {
            case kCevgPathCmd_MoveTo:  b.moveTo(np->coords[ci], np->coords[ci+1]); ci += 2; break;
            case kCevgPathCmd_LineTo:  b.lineTo(np->coords[ci], np->coords[ci+1]); ci += 2; break;
            case kCevgPathCmd_QuadTo:  b.quadTo(np->coords[ci], np->coords[ci+1],
                                                np->coords[ci+2], np->coords[ci+3]); ci += 4; break;
            case kCevgPathCmd_CubicTo: b.cubicTo(np->coords[ci], np->coords[ci+1],
                                                 np->coords[ci+2], np->coords[ci+3],
                                                 np->coords[ci+4], np->coords[ci+5]); ci += 6; break;
            case kCevgPathCmd_Close:   b.close(); break;
        }
    }
    return b.detach();
}

static SkMatrix ToSkMatrix(const float m[9]) {
    /* Cevg layout: column-major [sx, ky, 0, kx, sy, 0, tx, ty, 1].
     * x' = sx*x + kx*y + tx ; y' = ky*x + sy*y + ty. */
    SkMatrix sk;
    sk.setAll(m[0], m[3], m[6],
              m[1], m[4], m[7],
              0,    0,    1);
    return sk;
}

/* ===================================================================
 * Version / backend name
 * =================================================================== */
extern "C" const char* cevg_version(void)      { return "0.2.0"; }
extern "C" const char* cevg_backend_name(void) {
#ifdef CEVG_VULKAN
    return g_cevg_cpu_mode ? "cpu" : "skia_graphite_vulkan";
#else
    return "cpu";
#endif
}

#ifdef CEVG_VULKAN
/* ===================================================================
 * CevgSwapchain — GPU-direct presentation
 * -------------------------------------------------------------------
 * This is the core fix versus the old backend. Instead of:
 *     submit(SyncToCpu::kYes) -> asyncRescaleAndReadPixels(whole frame)
 *     -> memcpy to CPU -> SDL_Texture upload -> SDL present
 * we render into an offscreen Graphite SkSurface, then on present we
 * acquire the next swapchain image, blit (vkCmdBlitImage) the rendered
 * VkImage into it, and vkQueuePresentKHR — all ordered by semaphores,
 * with submit(SyncToCpu::kNo). The CPU never waits on the GPU and the
 * pixels never leave VRAM.
 *
 * Why blit rather than render straight into the swapchain image: wrapping
 * the acquired swapchain VkImage as a Graphite BackendTexture every frame
 * and getting the queue-family / layout handoff exactly right is the most
 * driver-fragile path in Skia. Rendering into a stable Cevg-owned VkImage
 * and blitting is robust, costs one extra GPU copy (cheap, in-VRAM), and
 * keeps Graphite's surface lifetime simple. A future optimization can
 * wrap the swapchain image directly once it is proven on the target
 * drivers; the public API does not change if we do.
 *
 * NOTE: the per-frame Vulkan command recording (begin command buffer,
 * image barriers, blit, submit, present) is standard Vulkan and is the
 * one place that must match the host's Vulkan version/validation; it is
 * fully contained here.
 * =================================================================== */
struct CevgSwapchain {
    CevgContext*   ctx;
    VkSurfaceKHR   vkSurface;
    VkSwapchainKHR swapchain;
    VkFormat       format;
    VkColorSpaceKHR colorSpace;
    VkExtent2D     extent;
    CevgPresentMode presentMode;

    std::vector<VkImage>     images;        /* swapchain images */
    uint32_t                 imageCount;
    uint32_t                 currentIndex;  /* acquired this frame */

    /* The stable Cevg-owned render target the canvas draws into; blitted
     * into the acquired swapchain image at present. */
    VkImage        renderImage;
    VkDeviceMemory renderMemory;
    sk_sp<SkSurface> renderSurface;         /* Graphite surface wrapping renderImage */
    VkFormat       renderFormat;            /* render image format (matched to swapchain) */
    SkColorType    renderColorType;         /* matching Skia color type */

    /* Double-buffered per-frame sync.  Frame N uses frame_sync[N % 2],
     * allowing the GPU to work on the previous frame's blit while the
     * CPU records the next frame.  This raises throughput from ~1 frame
     * to ~2 frames in-flight. */
    static constexpr int kMaxInFlight = 2;
    struct FrameSync {
        VkCommandPool    cmdPool;
        VkCommandBuffer  cmdBuf;
        VkSemaphore      acquireSem;   /* signalled when swapchain image is ready */
        VkSemaphore      blitDoneSem;  /* signalled when blit finished, present waits on it */
        VkSemaphore      renderDoneSem;/* signalled by Graphite's render submit; the blit
                                        * waits on it so it never reads renderImage before
                                        * Graphite has finished writing it */
        VkFence          inFlight;     /* CPU waits on this only to recycle the cmd buffer */
    } frame[kMaxInFlight];
    int              frameIndex;        /* cycles 0..kMaxInFlight-1 */
    bool             acquired;          /* did we acquire successfully this frame */
};

/* ---- small Vulkan helpers ---- */
static uint32_t cevg_find_mem_type(CevgContext* ctx, uint32_t typeBits,
                                   VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    ctx->vk.GetPhysicalDeviceMemoryProperties(ctx->phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
        if ((typeBits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & props) == props) return i;
    }
    return UINT32_MAX;  /* no suitable memory type found */
}

static VkPresentModeKHR cevg_pick_present_mode(CevgContext* ctx, VkSurfaceKHR surf,
                                               CevgPresentMode want) {
    uint32_t n = 0;
    ctx->vk.GetPhysicalDeviceSurfacePresentModesKHR(ctx->phys, surf, &n, nullptr);
    std::vector<VkPresentModeKHR> modes(n);
    ctx->vk.GetPhysicalDeviceSurfacePresentModesKHR(ctx->phys, surf, &n, modes.data());
    VkPresentModeKHR target = VK_PRESENT_MODE_FIFO_KHR;
    if (want == kCevgPresentMode_Mailbox)   target = VK_PRESENT_MODE_MAILBOX_KHR;
    if (want == kCevgPresentMode_Immediate) target = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (auto m : modes) if (m == target) return target;
    return VK_PRESENT_MODE_FIFO_KHR;  /* always available */
}

/* Build/rebuild the VkSwapchainKHR and the Cevg-owned render target. */
static bool cevg_swapchain_build(CevgSwapchain* sc, int w, int h) {
    CevgContext* ctx = sc->ctx;
    VkSurfaceCapabilitiesKHR caps;
    ctx->vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->phys, sc->vkSurface, &caps);

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0xFFFFFFFF) { extent.width = (uint32_t)w; extent.height = (uint32_t)h; }
    if (extent.width == 0 || extent.height == 0) return false;

    /* Pick an SRGB-capable BGRA/RGBA format. */
    uint32_t fmtCount = 0;
    ctx->vk.GetPhysicalDeviceSurfaceFormatsKHR(ctx->phys, sc->vkSurface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtCount);
    ctx->vk.GetPhysicalDeviceSurfaceFormatsKHR(ctx->phys, sc->vkSurface, &fmtCount, fmts.data());
    VkSurfaceFormatKHR chosen = fmts.empty()
        ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
        : fmts[0];
    for (auto f : fmts) {
        if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { chosen = f; break; }
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = sc->vkSurface;
    ci.minImageCount = imageCount;
    ci.imageFormat = chosen.format;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    /* TRANSFER_DST so we can blit the rendered image into the swapchain image. */
    ci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = cevg_pick_present_mode(ctx, sc->vkSurface, sc->presentMode);
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = sc->swapchain;

    VkSwapchainKHR newSwap = VK_NULL_HANDLE;
    if (ctx->vk.CreateSwapchainKHR(ctx->device, &ci, nullptr, &newSwap) != VK_SUCCESS) return false;
    if (sc->swapchain != VK_NULL_HANDLE)
        ctx->vk.DestroySwapchainKHR(ctx->device, sc->swapchain, nullptr);
    sc->swapchain = newSwap;
    sc->format = chosen.format;
    sc->colorSpace = chosen.colorSpace;
    sc->extent = extent;

    ctx->vk.GetSwapchainImagesKHR(ctx->device, sc->swapchain, &sc->imageCount, nullptr);
    sc->images.resize(sc->imageCount);
    ctx->vk.GetSwapchainImagesKHR(ctx->device, sc->swapchain, &sc->imageCount, sc->images.data());

    /* Choose the render-target format to MATCH the swapchain image format,
     * so the present-time GPU copy is a passthrough (no red/blue swap). If
     * the swapchain uses a format Skia can't wrap as a surface, fall back to
     * RGBA and accept that present will need a swizzling blit (handled in
     * cevg_swapchain_present via VkFormatToSkColorType mismatch detection). */
    SkColorType renderCT = VkFormatToSkColorType(chosen.format);
    VkFormat renderVkFmt = chosen.format;
    if (renderCT == kUnknown_SkColorType) {
        renderCT = kRGBA_8888_SkColorType;
        renderVkFmt = VK_FORMAT_R8G8B8A8_UNORM;
    }
    sc->renderColorType = renderCT;
    sc->renderFormat = renderVkFmt;

    /* (Re)create the Cevg-owned render image + Graphite surface. */
    if (sc->renderSurface) sc->renderSurface.reset();
    if (sc->renderImage)  { ctx->vk.DestroyImage(ctx->device, sc->renderImage, nullptr);  sc->renderImage = VK_NULL_HANDLE; }
    if (sc->renderMemory) { ctx->vk.FreeMemory(ctx->device, sc->renderMemory, nullptr);    sc->renderMemory = VK_NULL_HANDLE; }

    VkImageCreateInfo ii{};
    ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = renderVkFmt;
    ii.extent = {extent.width, extent.height, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (ctx->vk.CreateImage(ctx->device, &ii, nullptr, &sc->renderImage) != VK_SUCCESS) return false;

    VkMemoryRequirements req;
    ctx->vk.GetImageMemoryRequirements(ctx->device, sc->renderImage, &req);
    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = cevg_find_mem_type(ctx, req.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (ai.memoryTypeIndex == UINT32_MAX) return false;
    if (ctx->vk.AllocateMemory(ctx->device, &ai, nullptr, &sc->renderMemory) != VK_SUCCESS) return false;
    ctx->vk.BindImageMemory(ctx->device, sc->renderImage, sc->renderMemory, 0);

    /* Wrap renderImage as a Graphite BackendTexture -> SkSurface so the
     * canvas draws straight into it. */
    graphite::VulkanTextureInfo texInfo;
    texInfo.fSampleCount = graphite::SampleCount::k1;
    texInfo.fMipmapped = skgpu::Mipmapped::kNo;
    texInfo.fFlags = 0;
    texInfo.fFormat = renderVkFmt;
    texInfo.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
    texInfo.fImageUsageFlags = ii.usage;
    texInfo.fSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    graphite::BackendTexture backendTex =
        graphite::BackendTextures::MakeVulkan(
            SkISize::Make((int)extent.width, (int)extent.height),
            texInfo,
            VK_IMAGE_LAYOUT_UNDEFINED,
            ctx->queueFamily,
            sc->renderImage,
            skgpu::VulkanAlloc{});

    sc->renderSurface = SkSurfaces::WrapBackendTexture(
        ctx->recorder.get(), backendTex, renderCT,
        ctx->colorSpace, /*surfaceProps=*/nullptr);

    return sc->renderSurface != nullptr;
}

static CevgSwapchain* cevg_swapchain_create(CevgContext* ctx, VkSurfaceKHR vkSurface,
                                            int w, int h, CevgPresentMode mode) {
    CevgSwapchain* sc = new (std::nothrow) CevgSwapchain();
    if (!sc) return nullptr;
    sc->ctx = ctx;
    sc->vkSurface = vkSurface;
    sc->swapchain = VK_NULL_HANDLE;
    sc->presentMode = mode;
    sc->renderImage = VK_NULL_HANDLE;
    sc->renderMemory = VK_NULL_HANDLE;
    sc->currentIndex = 0;
    sc->frameIndex = 0;
    sc->acquired = false;

    /* Create per-frame Vulkan objects for each in-flight slot. */
    for (int i = 0; i < CevgSwapchain::kMaxInFlight; i++) {
        auto& fs = sc->frame[i];
        fs.cmdPool = VK_NULL_HANDLE;
        fs.cmdBuf = VK_NULL_HANDLE;
        fs.acquireSem = VK_NULL_HANDLE;
        fs.blitDoneSem = VK_NULL_HANDLE;
        fs.renderDoneSem = VK_NULL_HANDLE;
        fs.inFlight = VK_NULL_HANDLE;

        VkCommandPoolCreateInfo pci{};
        pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pci.queueFamilyIndex = ctx->queueFamily;
        ctx->vk.CreateCommandPool(ctx->device, &pci, nullptr, &fs.cmdPool);

        VkCommandBufferAllocateInfo cbi{};
        cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbi.commandPool = fs.cmdPool;
        cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbi.commandBufferCount = 1;
        ctx->vk.AllocateCommandBuffers(ctx->device, &cbi, &fs.cmdBuf);

        VkSemaphoreCreateInfo si{}; si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        ctx->vk.CreateSemaphore(ctx->device, &si, nullptr, &fs.acquireSem);
        ctx->vk.CreateSemaphore(ctx->device, &si, nullptr, &fs.blitDoneSem);
        ctx->vk.CreateSemaphore(ctx->device, &si, nullptr, &fs.renderDoneSem);
        VkFenceCreateInfo fi{}; fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        ctx->vk.CreateFence(ctx->device, &fi, nullptr, &fs.inFlight);
    }

    if (!cevg_swapchain_build(sc, w, h)) {
        /* partial cleanup */
        for (int i = 0; i < CevgSwapchain::kMaxInFlight; i++) {
            auto& fs = sc->frame[i];
            if (fs.inFlight)     ctx->vk.DestroyFence(ctx->device, fs.inFlight, nullptr);
            if (fs.blitDoneSem)  ctx->vk.DestroySemaphore(ctx->device, fs.blitDoneSem, nullptr);
            if (fs.renderDoneSem) ctx->vk.DestroySemaphore(ctx->device, fs.renderDoneSem, nullptr);
            if (fs.acquireSem)   ctx->vk.DestroySemaphore(ctx->device, fs.acquireSem, nullptr);
            if (fs.cmdPool)      ctx->vk.DestroyCommandPool(ctx->device, fs.cmdPool, nullptr);
        }
        delete sc;
        return nullptr;
    }
    return sc;
}

static void cevg_swapchain_destroy(CevgSwapchain* sc) {
    if (!sc) return;
    CevgContext* ctx = sc->ctx;
    if (ctx && ctx->device) {
        ctx->vk.DeviceWaitIdle(ctx->device);
        sc->renderSurface.reset();
        if (sc->renderImage)  ctx->vk.DestroyImage(ctx->device, sc->renderImage, nullptr);
        if (sc->renderMemory) ctx->vk.FreeMemory(ctx->device, sc->renderMemory, nullptr);
        for (int i = 0; i < CevgSwapchain::kMaxInFlight; i++) {
            auto& fs = sc->frame[i];
            if (fs.inFlight)     ctx->vk.DestroyFence(ctx->device, fs.inFlight, nullptr);
            if (fs.blitDoneSem)  ctx->vk.DestroySemaphore(ctx->device, fs.blitDoneSem, nullptr);
            if (fs.renderDoneSem) ctx->vk.DestroySemaphore(ctx->device, fs.renderDoneSem, nullptr);
            if (fs.acquireSem)   ctx->vk.DestroySemaphore(ctx->device, fs.acquireSem, nullptr);
            if (fs.cmdPool)      ctx->vk.DestroyCommandPool(ctx->device, fs.cmdPool, nullptr);
        }
        if (sc->swapchain)    ctx->vk.DestroySwapchainKHR(ctx->device, sc->swapchain, nullptr);
    }
    delete sc;
}

/* Acquire the next image for this frame. Returns false on OUT_OF_DATE
 * (caller should resize). */
static bool cevg_swapchain_acquire(CevgSwapchain* sc) {
    CevgContext* ctx = sc->ctx;
    auto& fs = sc->frame[sc->frameIndex];
    ctx->vk.WaitForFences(ctx->device, 1, &fs.inFlight, VK_TRUE, UINT64_MAX);
    VkResult r = ctx->vk.AcquireNextImageKHR(ctx->device, sc->swapchain, UINT64_MAX,
                                       fs.acquireSem, VK_NULL_HANDLE, &sc->currentIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) { sc->acquired = false; return false; }
    sc->acquired = (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR);
    return sc->acquired;
}

static void cevg_image_barrier(CevgContext* ctx, VkCommandBuffer cb, VkImage img,
                               VkImageLayout from, VkImageLayout to,
                               VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                               VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkImageMemoryBarrier b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.srcAccessMask = srcAccess;
    b.dstAccessMask = dstAccess;
    ctx->vk.CmdPipelineBarrier(cb, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &b);
}

/* Blit the Cevg render image into the acquired swapchain image and
 * present. Ordered by acquireSem (wait) and blitDoneSem (present wait). */
static CevgResult cevg_swapchain_present(CevgSwapchain* sc,
                                         const std::vector<CevgDirtyRect>& dirty) {
    CevgContext* ctx = sc->ctx;
    if (!sc->acquired) return kCevgErrorDeviceLost;

    auto& fs = sc->frame[sc->frameIndex];

    ctx->vk.ResetFences(ctx->device, 1, &fs.inFlight);
    ctx->vk.ResetCommandBuffer(fs.cmdBuf, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    ctx->vk.BeginCommandBuffer(fs.cmdBuf, &bi);

    /* GPU profiler: write begin timestamp. */
    if (ctx->profilerEnabled && ctx->profilerQueryPool != VK_NULL_HANDLE) {
        ctx->vk.CmdWriteTimestamp(fs.cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            ctx->profilerQueryPool, ctx->profilerRingIndex * 2);
    }

    VkImage dst = sc->images[sc->currentIndex];

    /* Graphite left renderImage in COLOR_ATTACHMENT_OPTIMAL after submit;
     * transition it to TRANSFER_SRC, and the swapchain image to TRANSFER_DST. */
    cevg_image_barrier(ctx, fs.cmdBuf, sc->renderImage,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    cevg_image_barrier(ctx, fs.cmdBuf, dst,
                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       0, VK_ACCESS_TRANSFER_WRITE_BIT,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    /* Transfer the rendered image into the swapchain image. When the render
     * image format matches the swapchain format (the normal case, since
     * cevg_swapchain_build matches them), use vkCmdCopyImage: it is a
     * bit-exact, filter-free, faster transfer with no channel swizzle. Only
     * if the formats differ (rare fallback path) do we blit, which can do
     * format conversion. Sizes always match here (render image is rebuilt to
     * the swapchain extent), so no scaling is required either way. */
    const bool formatsMatch = (sc->renderFormat == sc->format);

    auto copy_region = [&](int x, int y, int w, int h) {
        VkImageCopy region{};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.srcOffset = {x, y, 0};
        region.dstOffset = {x, y, 0};
        region.extent = {(uint32_t)w, (uint32_t)h, 1};
        ctx->vk.CmdCopyImage(fs.cmdBuf,
                       sc->renderImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &region);
    };
    auto blit_region = [&](int x, int y, int w, int h) {
        VkImageBlit region{};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.srcOffsets[0] = {x, y, 0};
        region.srcOffsets[1] = {x + w, y + h, 1};
        region.dstOffsets[0] = {x, y, 0};
        region.dstOffsets[1] = {x + w, y + h, 1};
        ctx->vk.CmdBlitImage(fs.cmdBuf,
                       sc->renderImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &region, VK_FILTER_NEAREST);
    };
    auto transfer_region = [&](int x, int y, int w, int h) {
        if (formatsMatch) copy_region(x, y, w, h);
        else              blit_region(x, y, w, h);
    };

    /* Always blit the full frame. Partial blits with dirty rects are
     * incorrect when the swapchain has multiple images: the acquired image
     * may contain content from several frames ago, and only updating the
     * dirty region would leave stale pixels from a different frame in the
     * non-dirty areas. Full blit is cheap (one GPU copy in VRAM) and
     * correct. The dirty-rect API is kept for future per-image damage
     * tracking but currently ignored. */
    transfer_region(0, 0, (int)sc->extent.width, (int)sc->extent.height);

    /* swapchain image -> PRESENT_SRC ; render image back to COLOR_ATTACHMENT. */
    cevg_image_barrier(ctx, fs.cmdBuf, dst,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                       VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    cevg_image_barrier(ctx, fs.cmdBuf, sc->renderImage,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    /* GPU profiler: write end timestamp. */
    if (ctx->profilerEnabled && ctx->profilerQueryPool != VK_NULL_HANDLE) {
        ctx->vk.CmdWriteTimestamp(fs.cmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            ctx->profilerQueryPool, ctx->profilerRingIndex * 2 + 1);
    }

    ctx->vk.EndCommandBuffer(fs.cmdBuf);

    /* The blit must wait on TWO things before its TRANSFER stage runs:
     *   - acquireSem:    the swapchain image is ready to be written, and
     *   - renderDoneSem: Graphite has finished rendering into renderImage
     *                    (signalled by the insertRecording in cevg_surface_present).
     * Both wait at the TRANSFER stage (the copy reads renderImage and writes
     * the swapchain image). Without renderDoneSem the copy could read
     * renderImage while Graphite was still drawing it — a GPU race that tears. */
    VkSemaphore          waitSems[2]   = { fs.acquireSem, fs.renderDoneSem };
    VkPipelineStageFlags waitStages[2] = { VK_PIPELINE_STAGE_TRANSFER_BIT,
                                           VK_PIPELINE_STAGE_TRANSFER_BIT };
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 2;
    submit.pWaitSemaphores = waitSems;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &fs.cmdBuf;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &fs.blitDoneSem;
    ctx->vk.QueueSubmit(ctx->queue, 1, &submit, fs.inFlight);

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &fs.blitDoneSem;
    present.swapchainCount = 1;
    present.pSwapchains = &sc->swapchain;
    present.pImageIndices = &sc->currentIndex;
    VkResult pr = ctx->vk.QueuePresentKHR(ctx->queue, &present);
    sc->acquired = false;
    sc->frameIndex = (sc->frameIndex + 1) % CevgSwapchain::kMaxInFlight;
    /* Advance the profiler ring index here — this is the single source of
     * truth for "the frame's GPU work has been submitted." The begin/end
     * timestamps were written in this command buffer, so they bracket the
     * actual blit+present GPU work. */
    ctx->profilerRingIndex = (ctx->profilerRingIndex + 1) % CevgContext::kProfilerRingSize;
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) return kCevgErrorDeviceLost;
    return (pr == VK_SUCCESS) ? kCevgSuccess : kCevgErrorOOM;
}
#endif /* CEVG_VULKAN */

/* ===================================================================
 * Portable font manager (FreeType-based, all platforms)
 *
 * Uses SkFontMgr_New_Custom_Directory to scan the platform's system
 * font directory with FreeType rendering.  Falls back to an empty
 * font manager if the directory does not exist.
 *
 * Platform font directories:
 *   Windows:   C:\Windows\Fonts
 *   macOS:     /System/Library/Fonts  +  /Library/Fonts
 *   iOS:       /System/Library/Fonts
 *   Linux:     /usr/share/fonts
 *   Android:   /system/fonts
 * =================================================================== */
static const char* cevg_system_font_dir() {
#if defined(SK_BUILD_FOR_WIN)
    return "C:\\Windows\\Fonts";
#elif defined(SK_BUILD_FOR_MAC)
    return "/System/Library/Fonts";
#elif defined(SK_BUILD_FOR_IOS)
    return "/System/Library/Fonts";
#elif defined(SK_BUILD_FOR_ANDROID)
    return "/system/fonts";
#else   /* Linux / other Unix */
    return "/usr/share/fonts";
#endif
}

static sk_sp<SkFontMgr> cevg_make_fontmgr() {
    /* Use the platform's native FontMgr — it has full fallback support
     * (onMatchFamilyStyleCharacter) and internally uses SkTypeface_proxy
     * to delegate glyph rendering to FreeType when SK_TYPEFACE_FACTORY_FREETYPE
     * is defined. */
#if defined(SK_BUILD_FOR_WIN)
    return SkFontMgr_New_DirectWrite();
#elif defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_IOS)
    return SkFontMgr_New_CoreText();
#elif defined(SK_BUILD_FOR_ANDROID)
    return SkFontMgr_New_AndroidNDK();
#elif defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    /* Fallback: FreeType Custom Directory (no platform FontMgr available) */
    const char* dir = cevg_system_font_dir();
    if (dir && dir[0]) {
        return SkFontMgr_New_Custom_Directory(dir);
    }
    return SkFontMgr_New_Custom_Empty();
#endif
}

/* ===================================================================
 * Context
 * =================================================================== */

/* ---- GPU device factory (Vulkan) ---- */
#ifdef CEVG_VULKAN
extern "C" CevgGpuDevice* cevg_gpu_device_create_vulkan(const CevgVulkanDevice* vk) {
    if (!vk) return nullptr;
    /* Version/size guard: the caller must at least reach vk_queue_index,
     * the last of the always-required fields. CEVG_HAS_FIELD lets newer
     * optional fields (e.g. vk_get_instance_proc_addr) be absent in an
     * older caller's struct. */
    if (!CEVG_HAS_FIELD(CevgVulkanDevice, vk->struct_size, vk_queue_index))
        return nullptr;
    if (!vk->vk_instance || !vk->vk_physical_device || !vk->vk_device || !vk->vk_queue)
        return nullptr;

    CevgGpuDevice* d = new (std::nothrow) CevgGpuDevice_();
    if (!d) return nullptr;
    d->backend     = CevgGpuBackend::Vulkan;
    d->instance    = reinterpret_cast<VkInstance>(vk->vk_instance);
    d->phys        = reinterpret_cast<VkPhysicalDevice>(vk->vk_physical_device);
    d->device      = reinterpret_cast<VkDevice>(vk->vk_device);
    d->queue       = reinterpret_cast<VkQueue>(vk->vk_queue);
    d->queueFamily = vk->vk_queue_index;

    void* gipa = CEVG_HAS_FIELD(CevgVulkanDevice, vk->struct_size, vk_get_instance_proc_addr)
                 ? vk->vk_get_instance_proc_addr : nullptr;

    /* Initialize glad2 on-demand loading so that internal vk* calls work.
     * The app provides vkGetInstanceProcAddr; we wire it into glad2's
     * global on-demand loader so that any vk* function used by cevg or
     * Skia is resolved lazily through it.
     *
     * glad2's internal loader already dlopens vulkan-1.dll and calls
     * vkGetInstanceProcAddr internally.  By calling
     * gladLoaderSetVulkanInstance/Device we tell it which instance and
     * device to use for function resolution, so device-level functions
     * are loaded via vkGetDeviceProcAddr (faster dispatch). */
    gladLoaderSetVulkanInstance(d->instance);
    gladLoaderSetVulkanDevice(d->device);

    d->getInstanceProc = gipa
        ? reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa)
        : vkGetInstanceProcAddr;  /* glad2 on-demand (initialized above) */

    if (CEVG_HAS_FIELD(CevgVulkanDevice, vk->struct_size, enabled_ext_count) &&
        vk->enabled_ext_count > 0 && vk->enabled_ext_names) {
        d->enabledExt.assign(vk->enabled_ext_names,
                             vk->enabled_ext_names + vk->enabled_ext_count);
    }
    d->apiVersion = CEVG_HAS_FIELD(CevgVulkanDevice, vk->struct_size, api_version)
                    ? vk->api_version : 0;
    return d;
}
#endif

extern "C" void cevg_gpu_device_destroy(CevgGpuDevice* device) {
    /* The host owns the underlying Vulkan objects; we only free our record. */
    delete device;
}

/* CPU-raster fallback "device". Carries no GPU handles; a context created from
 * it renders entirely on the CPU via Skia's raster pipeline. The host uses this
 * when no usable Vulkan device is available (headless, software-only, or a
 * failed GPU init): try cevg_gpu_device_create_vulkan first, fall back to this. */
extern "C" CevgGpuDevice* cevg_gpu_device_create_cpu(void) {
    g_cevg_cpu_mode = true;
    CevgGpuDevice* d = new (std::nothrow) CevgGpuDevice_();
    if (!d) return nullptr;
    d->backend = CevgGpuBackend::CPU;   /* all Vulkan fields stay null (value-init) */
    return d;
}

extern "C" CevgGpuDevice* cevg_gpu_device_create_auto(void) {
#ifdef CEVG_VULKAN
    /* Try Vulkan with default device - not possible without host VK setup,
     * so just fall through to CPU. Callers who have VK should use
     * cevg_gpu_device_create_vulkan() explicitly. */
#endif
    return cevg_gpu_device_create_cpu();
}

#ifdef CEVG_VULKAN
/* Resolve every Vulkan entry point Cevg calls directly, from the host's
 * vkGetInstanceProcAddr. Instance/global commands go through gipa; device-level
 * commands through vkGetDeviceProcAddr(device,...). Returns false if the device
 * loader or any command the swapchain/upload path cannot run without is missing.
 * Must be called after ctx->instance/device/getInstanceProc are set and before
 * any ctx->vk.* use; it populates ctx->vk and must not read it. */
static bool cevg_vk_api_load(CevgContext* ctx) {
    PFN_vkGetInstanceProcAddr gipa = ctx->getInstanceProc;
    if (!gipa) return false;
    PFN_vkGetDeviceProcAddr gdpa =
        (PFN_vkGetDeviceProcAddr)gipa(ctx->instance, "vkGetDeviceProcAddr");
    if (!gdpa) return false;

#define CEVG_LOAD_INST(name) \
    ctx->vk.name = (PFN_vk##name)gipa(ctx->instance, "vk" #name)
#define CEVG_LOAD_DEV(name) \
    ctx->vk.name = (PFN_vk##name)gdpa(ctx->device, "vk" #name)

    /* Global command: load with a null instance (valid per spec; some loaders
     * return null for it when given a real instance). */
    ctx->vk.EnumerateInstanceExtensionProperties =
        (PFN_vkEnumerateInstanceExtensionProperties)
        gipa(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");

    CEVG_LOAD_INST(EnumerateDeviceExtensionProperties);
    CEVG_LOAD_INST(GetPhysicalDeviceProperties);
    CEVG_LOAD_INST(GetPhysicalDeviceMemoryProperties);
    CEVG_LOAD_INST(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    CEVG_LOAD_INST(GetPhysicalDeviceSurfaceFormatsKHR);
    CEVG_LOAD_INST(GetPhysicalDeviceSurfacePresentModesKHR);

    CEVG_LOAD_DEV(DeviceWaitIdle);
    CEVG_LOAD_DEV(CreateSwapchainKHR);
    CEVG_LOAD_DEV(DestroySwapchainKHR);
    CEVG_LOAD_DEV(GetSwapchainImagesKHR);
    CEVG_LOAD_DEV(AcquireNextImageKHR);
    CEVG_LOAD_DEV(QueuePresentKHR);
    CEVG_LOAD_DEV(CreateImage);
    CEVG_LOAD_DEV(DestroyImage);
    CEVG_LOAD_DEV(GetImageMemoryRequirements);
    CEVG_LOAD_DEV(AllocateMemory);
    CEVG_LOAD_DEV(FreeMemory);
    CEVG_LOAD_DEV(BindImageMemory);
    CEVG_LOAD_DEV(CreateSemaphore);
    CEVG_LOAD_DEV(DestroySemaphore);
    CEVG_LOAD_DEV(CreateFence);
    CEVG_LOAD_DEV(DestroyFence);
    CEVG_LOAD_DEV(WaitForFences);
    CEVG_LOAD_DEV(ResetFences);
    CEVG_LOAD_DEV(CreateCommandPool);
    CEVG_LOAD_DEV(DestroyCommandPool);
    CEVG_LOAD_DEV(AllocateCommandBuffers);
    CEVG_LOAD_DEV(ResetCommandBuffer);
    CEVG_LOAD_DEV(BeginCommandBuffer);
    CEVG_LOAD_DEV(EndCommandBuffer);
    CEVG_LOAD_DEV(QueueSubmit);
    CEVG_LOAD_DEV(CmdPipelineBarrier);
    CEVG_LOAD_DEV(CmdCopyImage);
    CEVG_LOAD_DEV(CmdBlitImage);
    CEVG_LOAD_DEV(CmdWriteTimestamp);
    CEVG_LOAD_DEV(CreateQueryPool);
    CEVG_LOAD_DEV(DestroyQueryPool);
    CEVG_LOAD_DEV(ResetQueryPool);
    CEVG_LOAD_DEV(GetQueryPoolResults);

#undef CEVG_LOAD_INST
#undef CEVG_LOAD_DEV

    /* The swapchain / present / image-upload path cannot function without these.
     * The GPU profiler's query-pool commands are NOT required here: the profiler
     * is opt-in and cevg_gpu_profiler_enable guards on them separately, so a
     * device without VK_EXT_host_query_reset simply runs without profiling
     * rather than failing context creation. */
    PFN_vkVoidFunction required[] = {
        (PFN_vkVoidFunction)ctx->vk.EnumerateInstanceExtensionProperties,
        (PFN_vkVoidFunction)ctx->vk.EnumerateDeviceExtensionProperties,
        (PFN_vkVoidFunction)ctx->vk.GetPhysicalDeviceProperties,
        (PFN_vkVoidFunction)ctx->vk.GetPhysicalDeviceMemoryProperties,
        (PFN_vkVoidFunction)ctx->vk.GetPhysicalDeviceSurfaceCapabilitiesKHR,
        (PFN_vkVoidFunction)ctx->vk.GetPhysicalDeviceSurfaceFormatsKHR,
        (PFN_vkVoidFunction)ctx->vk.GetPhysicalDeviceSurfacePresentModesKHR,
        (PFN_vkVoidFunction)ctx->vk.DeviceWaitIdle,
        (PFN_vkVoidFunction)ctx->vk.CreateSwapchainKHR,
        (PFN_vkVoidFunction)ctx->vk.DestroySwapchainKHR,
        (PFN_vkVoidFunction)ctx->vk.GetSwapchainImagesKHR,
        (PFN_vkVoidFunction)ctx->vk.AcquireNextImageKHR,
        (PFN_vkVoidFunction)ctx->vk.QueuePresentKHR,
        (PFN_vkVoidFunction)ctx->vk.CreateImage,
        (PFN_vkVoidFunction)ctx->vk.DestroyImage,
        (PFN_vkVoidFunction)ctx->vk.GetImageMemoryRequirements,
        (PFN_vkVoidFunction)ctx->vk.AllocateMemory,
        (PFN_vkVoidFunction)ctx->vk.FreeMemory,
        (PFN_vkVoidFunction)ctx->vk.BindImageMemory,
        (PFN_vkVoidFunction)ctx->vk.CreateSemaphore,
        (PFN_vkVoidFunction)ctx->vk.DestroySemaphore,
        (PFN_vkVoidFunction)ctx->vk.CreateFence,
        (PFN_vkVoidFunction)ctx->vk.DestroyFence,
        (PFN_vkVoidFunction)ctx->vk.WaitForFences,
        (PFN_vkVoidFunction)ctx->vk.ResetFences,
        (PFN_vkVoidFunction)ctx->vk.CreateCommandPool,
        (PFN_vkVoidFunction)ctx->vk.DestroyCommandPool,
        (PFN_vkVoidFunction)ctx->vk.AllocateCommandBuffers,
        (PFN_vkVoidFunction)ctx->vk.ResetCommandBuffer,
        (PFN_vkVoidFunction)ctx->vk.BeginCommandBuffer,
        (PFN_vkVoidFunction)ctx->vk.EndCommandBuffer,
        (PFN_vkVoidFunction)ctx->vk.QueueSubmit,
        (PFN_vkVoidFunction)ctx->vk.CmdPipelineBarrier,
        (PFN_vkVoidFunction)ctx->vk.CmdCopyImage,
        (PFN_vkVoidFunction)ctx->vk.CmdBlitImage,
    };
    for (PFN_vkVoidFunction fn : required) if (!fn) return false;
    return true;
}
#endif

extern "C" CevgContext* cevg_context_create(const CevgConfig* config) {
    if (!config) return nullptr;
    /* The caller must reach at least `device`, the only required field. */
    if (!CEVG_HAS_FIELD(CevgConfig, config->struct_size, device)) return nullptr;
    CevgGpuDevice* dev = config->device;
    if (!dev) return nullptr;

    /* ---- CPU raster fallback ----
     * No Vulkan, no Graphite, no recorder. The drawing layer is pure SkCanvas
     * and works identically; only surface creation (SkSurfaces::Raster) and
     * pixel readback (synchronous SkSurface::readPixels) differ. Every
     * GPU-only entry point guards on ctx->ctx / ctx->isCpu and either no-ops or
     * returns kCevgErrorUnsupported on a CPU context. */
    if (dev->backend == CevgGpuBackend::CPU) {
        CevgContext* ctx = new (std::nothrow) CevgContext_();
        if (!ctx) return nullptr;
        ctx->isCpu = true;
        ctx->cevg_cs = CEVG_HAS_FIELD(CevgConfig, config->struct_size, color_space)
                       ? config->color_space : kCevgColorSpace_sRGB;
        int gcache = CEVG_HAS_FIELD(CevgConfig, config->struct_size, max_glyph_cache_size_kb)
                     ? config->max_glyph_cache_size_kb : 0;
        ctx->glyph_cache_kb = gcache > 0 ? gcache : 4096;
        ctx->colorSpace = (ctx->cevg_cs == kCevgColorSpace_Linear)
            ? SkColorSpace::MakeSRGBLinear() : SkColorSpace::MakeSRGB();
        ctx->fontMgr = cevg_make_fontmgr();
        SkGraphics::SetFontCacheLimit((size_t)ctx->glyph_cache_kb * 1024);
        /* ctx->ctx / recorder / Vulkan handles / vk table all stay null. */
        return ctx;
    }

    if (dev->backend != CevgGpuBackend::Vulkan) return nullptr;  /* unknown backend */

#ifdef CEVG_VULKAN
    CevgContext* ctx = new (std::nothrow) CevgContext_();
    if (!ctx) return nullptr;

    ctx->instance    = dev->instance;
    ctx->phys        = dev->phys;
    ctx->device      = dev->device;
    ctx->queue       = dev->queue;
    ctx->queueFamily = dev->queueFamily;
    ctx->getInstanceProc = dev->getInstanceProc;

    /* Resolve the Vulkan entry points Cevg calls directly (everything below
     * goes through ctx->vk.*). Must happen before the extension enumeration. */
    if (!cevg_vk_api_load(ctx)) { delete ctx; return nullptr; }

    /* Optional config fields: substitute documented defaults when the
     * caller's (older) struct_size does not cover them. */
    ctx->cevg_cs = CEVG_HAS_FIELD(CevgConfig, config->struct_size, color_space)
                   ? config->color_space : kCevgColorSpace_sRGB;
    ctx->sample_count = CEVG_HAS_FIELD(CevgConfig, config->struct_size, sample_count)
                        ? config->sample_count : 0;
    int gcache = CEVG_HAS_FIELD(CevgConfig, config->struct_size, max_glyph_cache_size_kb)
                 ? config->max_glyph_cache_size_kb : 0;
    ctx->glyph_cache_kb = gcache > 0 ? gcache : 4096;
    int vbudget = CEVG_HAS_FIELD(CevgConfig, config->struct_size, max_vram_cache_size_kb)
                  ? config->max_vram_cache_size_kb : 0;
    ctx->vram_budget = (size_t)(vbudget > 0 ? vbudget : 262144) * 1024;

    ctx->colorSpace = (ctx->cevg_cs == kCevgColorSpace_Linear)
        ? SkColorSpace::MakeSRGBLinear()
        : SkColorSpace::MakeSRGB();

    /* Build the Graphite Vulkan backend context from the host's device. */
    skgpu::VulkanBackendContext backendCtx = {};
    backendCtx.fInstance           = ctx->instance;
    backendCtx.fPhysicalDevice     = ctx->phys;
    backendCtx.fDevice             = ctx->device;
    backendCtx.fQueue              = ctx->queue;
    backendCtx.fGraphicsQueueIndex = ctx->queueFamily;
    /* Use the API version the host created its instance with, when provided;
     * otherwise keep the conservative 1.1 baseline. */
    backendCtx.fMaxAPIVersion      = dev->apiVersion ? dev->apiVersion
                                                     : VK_API_VERSION_1_1;

    PFN_vkGetInstanceProcAddr gipa = ctx->getInstanceProc;
    VkInstance capturedInstance = ctx->instance;
    backendCtx.fGetProc = [gipa, capturedInstance](const char* name, VkInstance inst, VkDevice dev)
        -> PFN_vkVoidFunction {
        /* Skia may pass inst=NULL when querying device-level functions.
         * Use the captured instance to get vkGetDeviceProcAddr. */
        VkInstance useInst = inst ? inst : capturedInstance;
        if (dev) {
            auto gdpa = (PFN_vkGetDeviceProcAddr)gipa(useInst, "vkGetDeviceProcAddr");
            if (gdpa) {
                auto fn = gdpa(dev, name);
                if (fn) return fn;
            }
        }
        return gipa(useInst, name);
    };

    /* Enumerate extensions for VulkanExtensions::init. */
    uint32_t instExtCount = 0;
    ctx->vk.EnumerateInstanceExtensionProperties(nullptr, &instExtCount, nullptr);
    std::vector<VkExtensionProperties> instExtProps(instExtCount);
    std::vector<const char*> instExtNames(instExtCount);
    if (instExtCount) {
        ctx->vk.EnumerateInstanceExtensionProperties(nullptr, &instExtCount, instExtProps.data());
        for (uint32_t i = 0; i < instExtCount; i++) instExtNames[i] = instExtProps[i].extensionName;
    }
    uint32_t devExtCount = 0;
    const char** devExtNames = nullptr;
    std::vector<const char*> devExtStorage;
    std::vector<VkExtensionProperties> devExtProps;
    if (!dev->enabledExt.empty()) {
        devExtCount = (uint32_t)dev->enabledExt.size();
        devExtNames = dev->enabledExt.data();
    } else {
        ctx->vk.EnumerateDeviceExtensionProperties(ctx->phys, nullptr, &devExtCount, nullptr);
        devExtProps.resize(devExtCount);
        if (devExtCount) {
            ctx->vk.EnumerateDeviceExtensionProperties(ctx->phys, nullptr, &devExtCount, devExtProps.data());
            devExtStorage.resize(devExtCount);
            for (uint32_t i = 0; i < devExtCount; i++) devExtStorage[i] = devExtProps[i].extensionName;
            devExtNames = devExtStorage.data();
        }
    }
    ctx->vkExtensions.init(backendCtx.fGetProc, ctx->instance, ctx->phys,
                           instExtCount, instExtNames.data(),
                           devExtCount, devExtNames);
    backendCtx.fVkExtensions = &ctx->vkExtensions;

    /* ThreadSafe::kYes: the public API supports recording on multiple worker
     * threads (one graphite::Recorder each). Recorders allocate GPU resources
     * (offscreen surfaces, image-texture uploads) through this allocator, so
     * those allocations can happen concurrently. A non-thread-safe allocator
     * would corrupt under that — which is exactly the advertised use case. */
    backendCtx.fMemoryAllocator =
        skgpu::VulkanMemoryAllocators::Make(backendCtx, skgpu::ThreadSafe::kYes);

    graphite::ContextOptions opts;
    ctx->ctx = graphite::ContextFactory::MakeVulkan(backendCtx, opts);
    if (!ctx->ctx) { delete ctx; return nullptr; }

    ctx->recorder = ctx->ctx->makeRecorder();
    if (!ctx->recorder) { delete ctx; return nullptr; }
    /* Apply the caller's VRAM budget (previously accepted but never enforced).
     * Graphite budgets are per-recorder, so worker recorders created via
     * cevg_recorder_create get the same cap. */
    ctx->recorder->setMaxBudgetedBytes(ctx->vram_budget);
    /* Apply the glyph-cache budget. NOTE: SkGraphics font cache is process-
     * global, so with multiple contexts the last one set wins — acceptable for
     * the single-context common case; revisit if per-context isolation is needed. */
    SkGraphics::SetFontCacheLimit((size_t)ctx->glyph_cache_kb * 1024);

    ctx->fontMgr = cevg_make_fontmgr();
    ctx->default_recorder_wrapper = nullptr;

    /* GPU profiler: initially disabled, query pool created lazily. */
    ctx->profilerEnabled = false;
    ctx->profilerQueryPool = VK_NULL_HANDLE;
    ctx->profilerRingIndex = 0;
    ctx->profilerHasData = false;
    ctx->profilerLastMs = 0.0f;
    ctx->timestampPeriod = 1.0f;
    /* Read timestamp period from the physical device. */
    VkPhysicalDeviceProperties devProps;
    ctx->vk.GetPhysicalDeviceProperties(ctx->phys, &devProps);
    ctx->timestampPeriod = devProps.limits.timestampPeriod;

    return ctx;
#else
    return nullptr;  /* Vulkan backend requested but CEVG_VULKAN not compiled in */
#endif
}

extern "C" void cevg_context_destroy(CevgContext* ctx) {
    if (!ctx) return;
#ifdef CEVG_VULKAN
    if (ctx->device) ctx->vk.DeviceWaitIdle(ctx->device);
#endif
    /* Free the default recorder wrapper (it borrows, not owns, the
     * graphite::Recorder, so just delete the wrapper shell). */
    delete ctx->default_recorder_wrapper;
#ifdef CEVG_VULKAN
    /* Destroy the profiler query pool. */
    if (ctx->profilerQueryPool != VK_NULL_HANDLE && ctx->device)
        ctx->vk.DestroyQueryPool(ctx->device, ctx->profilerQueryPool, nullptr);
#endif
    ctx->recorder.reset();
    ctx->ctx.reset();
    delete ctx;
}

extern "C" CevgResult cevg_context_reset(CevgContext* ctx) {
    if (!ctx || !ctx->ctx) return kCevgErrorInvalidArg;
    /* Recreate the recorder; the Graphite Context itself survives a
     * swapchain loss as long as the VkDevice does. Callers recreate
     * surfaces/swapchains afterward. */
    ctx->recorder = ctx->ctx->makeRecorder();
    return ctx->recorder ? kCevgSuccess : kCevgErrorDeviceLost;
}

extern "C" void cevg_context_tick(CevgContext* ctx) {
    if (!ctx || !ctx->ctx) return;
    /* Let Graphite reclaim resources whose GPU work has completed and
     * trim caches toward budget. */
    ctx->ctx->checkAsyncWorkCompletion();
    if (ctx->recorder) {
        ctx->recorder->performDeferredCleanup(std::chrono::milliseconds(0));
    }
}

extern "C" size_t cevg_context_get_vram_used(const CevgContext* ctx) {
    if (!ctx || !ctx->ctx) return 0;
    /* Graphite exposes the current budgeted bytes on the recorder.
     * This is the total GPU memory managed by Graphite, which is the
     * most useful metric for callers monitoring VRAM usage. */
    return ctx->recorder ? (size_t)ctx->recorder->currentBudgetedBytes() : 0;
}

extern "C" size_t cevg_context_get_vram_budget(const CevgContext* ctx) {
    return ctx ? ctx->vram_budget : 0;
}

/* ===================================================================
 * Recorder
 * =================================================================== */
extern "C" CevgRecorder* cevg_recorder_create(CevgContext* ctx) {
    if (!ctx) return nullptr;
    /* CPU backend: raster draws are immediate and per-surface thread-safe, so a
     * "recorder" carries no graphite::Recorder. It is a valid handle that lets
     * the same multi-threaded recording code (create recorder → surface →
     * draw → snap → insert) run unchanged; the draws simply rasterize on the
     * worker thread and snap/insert become no-ops. */
    if (ctx->isCpu) {
        CevgRecorder* r = new (std::nothrow) CevgRecorder_();
        if (!r) return nullptr;
        r->owned = nullptr;
        r->borrowed = nullptr;   /* get() == nullptr marks a CPU recorder (with ctx->isCpu) */
        r->ctx = ctx;
        return r;
    }
    if (!ctx->ctx) return nullptr;
    CevgRecorder* r = new (std::nothrow) CevgRecorder_();
    if (!r) return nullptr;
    r->owned = ctx->ctx->makeRecorder();
    if (!r->owned) { delete r; return nullptr; }
    r->owned->setMaxBudgetedBytes(ctx->vram_budget);  /* honor the context budget */
    r->borrowed = nullptr;
    r->ctx = ctx;
    return r;
}

extern "C" void cevg_recorder_destroy(CevgRecorder* rec) {
    if (!rec) return;
    /* If this is a borrowed wrapper (the context default), owned is null
     * and we must not free the underlying recorder. */
    rec->owned.reset();
    delete rec;
}

/* Expose the context's built-in recorder through a CevgRecorder handle.
 * Cached per context (in ctx->default_recorder_wrapper) so repeated calls
 * return the same wrapper; the wrapper borrows (does not own) the
 * graphite::Recorder and is freed when the context is destroyed. */
extern "C" CevgRecorder* cevg_context_default_recorder(CevgContext* ctx) {
    if (!ctx) return nullptr;
    if (!ctx->isCpu && !ctx->recorder) return nullptr;
    if (ctx->default_recorder_wrapper) return ctx->default_recorder_wrapper;
    CevgRecorder* r = new (std::nothrow) CevgRecorder_();
    if (!r) return nullptr;
    r->owned = nullptr;
    r->borrowed = ctx->isCpu ? nullptr : ctx->recorder.get();  /* CPU: no graphite recorder */
    r->ctx = ctx;
    ctx->default_recorder_wrapper = r;
    return r;
}

/* ---- Worker snap / insert (NEW) ---- */
extern "C" CevgRecording* cevg_recorder_snap(CevgRecorder* rec) {
    if (!rec || !rec->ctx) return nullptr;

    /* CPU backend: the worker already rasterized its draws directly onto its
     * surface. Take a snapshot so insert_recording() can blit it onto the
     * main surface. */
    if (rec->ctx->isCpu) {
        CevgRecording* out = new (std::nothrow) CevgRecording_();
        if (!out) return nullptr;
        if (rec->cpuSurface) {
            out->cpuSnapshot = rec->cpuSurface->makeImageSnapshot();
            out->cpuW = rec->cpuW;
            out->cpuH = rec->cpuH;
        }
        return out;
    }

    graphite::Recorder* r = rec->get();
    if (!r) return nullptr;

    /* De-register this recorder from the context's automatic present-time snap
     * set: a recorder snapped explicitly is the caller's to insert, and must NOT
     * be snapped again from the present/flush thread (which would be a
     * cross-thread snap of a recorder this thread owns). */
    {
        std::lock_guard<std::mutex> lock(rec->ctx->active_recorders_mutex);
        auto& v = rec->ctx->active_recorders;
        v.erase(std::remove(v.begin(), v.end(), r), v.end());
    }

    std::unique_ptr<graphite::Recording> recording = r->snap();
    if (!recording) return nullptr;   /* nothing was recorded */
    CevgRecording* out = new (std::nothrow) CevgRecording_();
    if (!out) return nullptr;
    out->recording = std::move(recording);
    return out;
}

extern "C" CevgResult cevg_context_insert_recording(CevgContext* ctx,
                                                    CevgRecording* recording) {
    if (!ctx || !recording) return kCevgErrorInvalidArg;
    /* CPU: blit the worker's snapshot onto the main (current) surface. */
    if (ctx->isCpu) {
        if (!recording->cpuSnapshot) return kCevgSuccess;
        /* Find the main surface to blit onto. We use the context's current
         * canvas if available, or the first surface in the active set. */
        SkCanvas* dst = nullptr;
        /* Walk active recorders to find a surface with a canvas we can draw on.
         * The simplest approach: use the context's default recorder surface if
         * it has one, otherwise just skip (nothing to blit onto). */
        if (ctx->default_recorder_wrapper && ctx->default_recorder_wrapper->cpuSurface) {
            dst = ctx->default_recorder_wrapper->cpuSurface->getCanvas();
        }
        if (dst && recording->cpuSnapshot) {
            dst->drawImage(recording->cpuSnapshot, 0, 0);
        }
        return kCevgSuccess;
    }
    if (!ctx->ctx || !recording->recording) return kCevgErrorInvalidArg;
    graphite::InsertRecordingInfo info;
    info.fRecording = recording->recording.get();
    ctx->ctx->insertRecording(info);
    return kCevgSuccess;
}

extern "C" void cevg_recording_destroy(CevgRecording* recording) {
    delete recording;
}

/* NOTE on recorder binding: in Graphite an SkSurface is created from a
 * specific Recorder and can ONLY be recorded through that recorder. Cevg
 * enforces this by storing each surface's owner recorder
 * (CevgSurface_::ownerRecorder) and having the canvas record into it. Real
 * multithreaded recording works by creating the SURFACE with the worker's
 * recorder (cevg_surface_create_with_recorder), drawing into it on that
 * thread, then compositing the snapshot on the GPU thread. Passing a foreign
 * recorder to cevg_canvas_create_with_recorder cannot redirect draws and is
 * therefore ignored in favor of the surface's true owner. */

/* ===================================================================
 * Image GPU upload helper
 * -------------------------------------------------------------------
 * Lazily uploads the CPU image to a Graphite texture, caching the result.
 * Thread-safe: multiple worker threads may draw the same image concurrently,
 * so the cache fill is guarded by the image's mutex with double-checked
 * locking. The upload uses the supplied recorder (the one the drawing canvas
 * records into) so the texture is created on the same recorder that will
 * consume it, avoiding cross-recorder/cross-thread use of the default
 * recorder. Once created, a Graphite SkImage is shareable across recorders
 * of the same Context, so the cache is valid for all subsequent draws.
 * =================================================================== */
static sk_sp<SkImage> EnsureGraphiteImageRec(const CevgImage* image, graphite::Recorder* rec) {
    if (!image || !image->image) return nullptr;
    /* Fast path: already uploaded. gpu_ready is acquire-loaded; once it reads
     * true, gpu_image was fully published (release store below) and is never
     * mutated again, so the lock-free read + sk_sp copy here is data-race-free.
     * (sk_sp's own refcount is atomic, so concurrent copies are fine.) */
    if (image->gpu_ready.load(std::memory_order_acquire)) return image->gpu_image;
    if (!rec) return image->image;  /* no recorder: fall back to CPU image */

    std::lock_guard<std::mutex> lock(image->gpu_upload_mutex);
    /* Re-check under the lock (relaxed is fine: the mutex provides ordering). */
    if (image->gpu_ready.load(std::memory_order_relaxed)) return image->gpu_image;
    sk_sp<SkImage> gpu = SkImages::TextureFromImage(rec, image->image.get());
    if (gpu) {
        image->gpu_image = gpu;                                   /* write-once */
        image->gpu_ready.store(true, std::memory_order_release);  /* publish */
        return gpu;
    }
    return image->image;  /* upload failed: fall back to CPU image */
}

/* Convenience overload used where only a CevgContext is on hand (the default
 * recorder is the right target there — these call sites run on the GPU
 * thread). */
static sk_sp<SkImage> EnsureGraphiteImage(const CevgImage* image, CevgContext* ctx) {
    return EnsureGraphiteImageRec(image, ctx && ctx->recorder ? ctx->recorder.get() : nullptr);
}

/* ===================================================================
 * Surface
 * =================================================================== */

/* Core offscreen surface creator bound to an explicit graphite::Recorder.
 * All public offscreen creators funnel through here so ownerRecorder is
 * always set correctly. */
static CevgSurface* cevg_surface_create_impl(CevgContext* ctx,
                                             graphite::Recorder* rec,
                                             int w, int h, const void* pixels) {
    if (!ctx || !rec || w <= 0 || h <= 0) return nullptr;

    SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType,
                                         kPremul_SkAlphaType, ctx->colorSpace);
    sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(rec, info);
    if (!surface) return nullptr;

    if (pixels) {
        SkPixmap src(info, pixels, info.minRowBytes());
        surface->writePixels(src, 0, 0);
    }

    CevgSurface* s = new (std::nothrow) CevgSurface_();
    if (!s) return nullptr;
    s->surface = std::move(surface);
    s->ctx = ctx;
    s->width = w;
    s->height = h;
    s->ownerRecorder = rec;
    s->swapchain = nullptr;
    return s;
}

/* CPU-raster offscreen surface (the fallback path). Same pixel format and
 * color space as the GPU offscreen surface, so drawing and readback behave
 * identically; it just isn't backed by a recorder. */
static CevgSurface* cevg_surface_create_raster(CevgContext* ctx, int w, int h,
                                               const void* pixels) {
    if (!ctx || w <= 0 || h <= 0) return nullptr;
    SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType,
                                         kPremul_SkAlphaType, ctx->colorSpace);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface) return nullptr;
    if (pixels) {
        SkPixmap src(info, pixels, info.minRowBytes());
        surface->writePixels(src, 0, 0);
    }
    CevgSurface* s = new (std::nothrow) CevgSurface_();
    if (!s) return nullptr;
    s->surface = std::move(surface);
    s->ctx = ctx;
    s->width = w;
    s->height = h;
    s->ownerRecorder = nullptr;   /* CPU: no recorder owns this surface */
    s->swapchain = nullptr;
    return s;
}

extern "C" CevgSurface* cevg_surface_create(CevgContext* ctx, int w, int h,
                                            const void* pixels) {
    if (!ctx) return nullptr;
    if (ctx->isCpu) return cevg_surface_create_raster(ctx, w, h, pixels);
    if (!ctx->recorder) return nullptr;
    return cevg_surface_create_impl(ctx, ctx->recorder.get(), w, h, pixels);
}

/* NEW (ABI-additive): offscreen surface bound to a caller-supplied recorder.
 * This is what enables genuine multithreaded recording: a worker thread
 * creates its own CevgRecorder, makes a surface from it here, and draws into
 * it without touching the default recorder or any other thread's recorder. */
extern "C" CevgSurface* cevg_surface_create_with_recorder(CevgRecorder* rec,
                                                          int w, int h,
                                                          const void* pixels) {
    if (!rec || !rec->ctx) return nullptr;
    /* CPU: create a raster surface and bind it to the recorder so that
     * snap() can take a snapshot for later compositing onto the main surface. */
    if (rec->ctx->isCpu) {
        CevgSurface* s = cevg_surface_create_raster(rec->ctx, w, h, pixels);
        if (s) {
            rec->cpuSurface = s->surface;   /* keep ref so snap can snapshot it */
            rec->cpuW = w;
            rec->cpuH = h;
        }
        return s;
    }
    graphite::Recorder* gr = rec->get();
    if (!gr) return nullptr;
    return cevg_surface_create_impl(rec->ctx, gr, w, h, pixels);
}

#ifdef CEVG_VULKAN
extern "C" CevgSurface* cevg_surface_create_for_vk_surface(
    CevgContext* ctx, void* vk_surface, int w, int h, CevgPresentMode mode) {
    if (!ctx || !vk_surface || w <= 0 || h <= 0) return nullptr;
    if (ctx->isCpu) return nullptr;   /* window present is GPU-only; use offscreen + read_pixels */

    CevgSwapchain* sc = cevg_swapchain_create(
        ctx, reinterpret_cast<VkSurfaceKHR>(vk_surface), w, h, mode);
    if (!sc) return nullptr;

    CevgSurface* s = new (std::nothrow) CevgSurface_();
    if (!s) { cevg_swapchain_destroy(sc); return nullptr; }
    s->ctx = ctx;
    s->width = (int)sc->extent.width;
    s->height = (int)sc->extent.height;
    s->swapchain = sc;
    s->surface = sc->renderSurface;   /* canvas draws into the Cevg render image */
    /* The swapchain render target is created from the context default
     * recorder (see cevg_swapchain_build), so a window canvas always records
     * into the default recorder. */
    s->ownerRecorder = ctx->recorder.get();
    return s;
}
#else
extern "C" CevgSurface* cevg_surface_create_for_vk_surface(
    CevgContext* ctx, void* vk_surface, int w, int h, CevgPresentMode mode) {
    (void)ctx; (void)vk_surface; (void)w; (void)h; (void)mode;
    return nullptr;  /* Vulkan not compiled in */
}
#endif

/* CPU presentable surface: render straight into a host-owned pixel buffer
 * (RGBA8888, premultiplied). The host owns the memory and keeps it alive for
 * the surface's lifetime. This is the CPU backend's equivalent of a window
 * surface — after drawing, call cevg_surface_present (a flush) and then blit /
 * show the buffer however the host likes (the standard software-present model).
 * Returns NULL on a GPU context (GPU presents via cevg_surface_create_for_vk_surface). */
extern "C" CevgSurface* cevg_surface_create_for_buffer(CevgContext* ctx, void* pixels,
                                                       int w, int h, int stride_bytes) {
    if (!ctx || !pixels || w <= 0 || h <= 0) return nullptr;
    if (!ctx->isCpu) return nullptr;   /* GPU presents through a Vulkan swapchain */
    SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType,
                                         kPremul_SkAlphaType, ctx->colorSpace);
    size_t rowBytes = stride_bytes > 0 ? (size_t)stride_bytes : info.minRowBytes();
    if (rowBytes < info.minRowBytes()) return nullptr;
    sk_sp<SkSurface> surface = SkSurfaces::WrapPixels(info, pixels, rowBytes);
    if (!surface) return nullptr;
    CevgSurface* s = new (std::nothrow) CevgSurface_();
    if (!s) return nullptr;
    s->surface = std::move(surface);
    s->ctx = ctx;
    s->width = w;
    s->height = h;
    s->ownerRecorder = nullptr;
    s->swapchain = nullptr;
    s->cpuPresentable = true;   /* cevg_surface_present succeeds on this surface */
    return s;
}

extern "C" CevgResult cevg_surface_resize(CevgSurface* surface, int w, int h) {
    if (!surface) return kCevgErrorInvalidArg;
    if (!surface->swapchain) return kCevgErrorUnsupported;  /* offscreen: recreate instead */
    if (w <= 0 || h <= 0) return kCevgErrorInvalidArg;
#ifdef CEVG_VULKAN
    CevgContext* ctx = surface->ctx;
    ctx->vk.DeviceWaitIdle(ctx->device);
    if (!cevg_swapchain_build(surface->swapchain, w, h)) return kCevgErrorDeviceLost;
    surface->width = (int)surface->swapchain->extent.width;
    surface->height = (int)surface->swapchain->extent.height;
    surface->surface = surface->swapchain->renderSurface;
    return kCevgSuccess;
#else
    return kCevgErrorUnsupported;
#endif
}

extern "C" void cevg_surface_destroy(CevgSurface* surface) {
    if (!surface) return;
    if (surface->swapchain) {
        surface->surface.reset();              /* drop our ref before swapchain frees the image */
#ifdef CEVG_VULKAN
        cevg_swapchain_destroy(surface->swapchain);
#endif
    } else {
        surface->surface.reset();
    }
    delete surface;
}

extern "C" int cevg_surface_get_width(const CevgSurface* s)  { return s ? s->width : 0; }
extern "C" int cevg_surface_get_height(const CevgSurface* s) { return s ? s->height : 0; }

extern "C" void cevg_surface_add_dirty_rect(CevgSurface* surface, int x, int y, int w, int h) {
    if (!surface || w <= 0 || h <= 0) return;
    surface->dirty.push_back(CevgDirtyRect{x, y, w, h});
}

/* GPU readback (tests/screenshots only — NOT the present path). */
struct GraphiteReadCtx { void* dst; size_t rowBytes; int height; bool success; };
static void graphite_read_cb(SkImage::ReadPixelsContext ctx,
                             std::unique_ptr<const SkImage::AsyncReadResult> result) {
    auto* rc = static_cast<GraphiteReadCtx*>(ctx);
    if (result && result->count() > 0) {
        size_t srcRB = result->rowBytes(0);
        const char* src = static_cast<const char*>(result->data(0));
        char* dst = static_cast<char*>(rc->dst);
        size_t copy = rc->rowBytes < srcRB ? rc->rowBytes : srcRB;
        for (int y = 0; y < rc->height; y++)
            memcpy(dst + y * rc->rowBytes, src + y * srcRB, copy);
        rc->success = true;
    } else rc->success = false;
}

extern "C" CevgResult cevg_surface_read_pixels(CevgSurface* surface, void* out_pixels) {
    if (!surface || !surface->surface || !out_pixels) return kCevgErrorInvalidArg;
    CevgContext* ctx = surface->ctx;
    if (!ctx) return kCevgErrorUnsupported;

    SkImageInfo info = surface->surface->imageInfo();

    /* CPU raster: the pixels already live in the surface's backing store, so
     * read them back synchronously — no snap/submit/GPU readback. Same
     * premultiplied RGBA layout as the GPU path, so callers see no difference. */
    if (ctx->isCpu) {
        return surface->surface->readPixels(info, out_pixels, info.minRowBytes(), 0, 0)
               ? kCevgSuccess : kCevgErrorOOM;
    }
    if (!ctx->ctx) return kCevgErrorUnsupported;

    /* 1. Snap all pending recordings and submit WITHOUT blocking.
     *    This ensures all rendering commands are queued to the GPU. */
    cevg_context_snap_and_submit(ctx, graphite::SyncToCpu::kNo);

    /* 2. Now enqueue the async readback and submit WITH blocking.
     *    The previous submit(kNo) ensures the GPU has all the draw work;
     *    this submit(kYes) waits for both the draws and the readback. */
    GraphiteReadCtx rc{out_pixels, info.minRowBytes(), info.height(), false};
    ctx->ctx->asyncRescaleAndReadPixels(
        surface->surface.get(), info, SkIRect::MakeWH(info.width(), info.height()),
        SkImage::RescaleGamma::kSrc, SkImage::RescaleMode::kNearest,
        graphite_read_cb, &rc);
    ctx->ctx->submit(graphite::SyncToCpu::kYes);
    return rc.success ? kCevgSuccess : kCevgErrorOOM;
}

/* GPU-direct present: snap + submit(kNo) + swapchain blit/present. */
extern "C" CevgResult cevg_surface_present(CevgSurface* surface) {
    if (!surface) return kCevgErrorInvalidArg;
    CevgContext* ctx = surface->ctx;
    /* CPU: raster drew straight into the surface's backing store. For a
     * presentable (host-buffer) surface the pixels are already in the host's
     * memory, so present is a successful no-op; the host then displays the
     * buffer. An offscreen CPU surface is not presentable (use read_pixels),
     * matching the GPU backend where offscreen surfaces return unsupported. */
    if (ctx && ctx->isCpu) {
        return surface->cpuPresentable ? kCevgSuccess : kCevgErrorUnsupported;
    }
    if (!surface->swapchain) return kCevgErrorUnsupported;
    if (!ctx || !ctx->ctx) return kCevgErrorInvalidArg;

#ifdef CEVG_VULKAN
    CevgSwapchain* sc = surface->swapchain;
    auto& fs = sc->frame[sc->frameIndex];   /* same frame the blit will use */
    graphite::Recorder* owner = surface->ownerRecorder;  /* window draws land here */

    /* 1a. Snap every OTHER active recorder first and insert (no submit yet).
     *     These are worker/offscreen recorders; inserting them before the
     *     window recording keeps cross-recorder order correct (a producer
     *     offscreen surface is rendered before the window composites it). */
    {
        std::vector<graphite::Recorder*> recorders;
        {
            std::lock_guard<std::mutex> lock(ctx->active_recorders_mutex);
            recorders.swap(ctx->active_recorders);
        }
        for (auto* rec : recorders) {
            if (rec == owner) continue;  /* the window recorder is snapped below */
            if (auto recording = rec->snap()) {
                graphite::InsertRecordingInfo info;
                info.fRecording = recording.get();
                ctx->ctx->insertRecording(info);
            }
        }
    }

    /* 1b. Snap the WINDOW recorder and insert it with:
     *     - fSignalSemaphores = [renderDoneSem]: Graphite signals this when the
     *       render into renderImage has completed on the GPU. The blit waits on
     *       it (below) so it can never read renderImage early. This is the fix
     *       for the missing render->blit GPU sync.
     *     - fTargetTextureState = COLOR_ATTACHMENT_OPTIMAL: pins the layout
     *       renderImage is left in after submit, so the blit's first barrier
     *       (which assumes COLOR_ATTACHMENT_OPTIMAL) is always valid instead of
     *       relying on an unspecified Graphite post-submit layout. */
    if (owner) {
        if (auto recording = owner->snap()) {
            skgpu::MutableTextureState targetState =
                skgpu::MutableTextureStates::MakeVulkan(
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, ctx->queueFamily);
            graphite::BackendSemaphore signalSem =
                graphite::BackendSemaphores::MakeVulkan(fs.renderDoneSem);
            graphite::InsertRecordingInfo info;
            info.fRecording          = recording.get();
            info.fTargetSurface      = sc->renderSurface.get();
            info.fTargetTextureState = &targetState;
            info.fNumSignalSemaphores = 1;
            info.fSignalSemaphores    = &signalSem;
            ctx->ctx->insertRecording(info);
        }
    }
    ctx->ctx->submit(graphite::SyncToCpu::kNo);

    /* 2. Blit the rendered image into the acquired swapchain image and
     *    present, ordered by GPU semaphores (acquireSem + renderDoneSem). */
    CevgResult r = cevg_swapchain_present(sc, surface->dirty);
    surface->dirty.clear();
    return r;
#else
    /* Without Vulkan, just flush the context. */
    cevg_context_snap_and_submit(ctx, graphite::SyncToCpu::kNo);
    surface->dirty.clear();
    return kCevgSuccess;
#endif
}

/* Register a recorder as active for this frame.  Thread-safe. */
static void cevg_context_register_recorder(CevgContext* ctx, graphite::Recorder* rec) {
    if (!ctx || !rec) return;
    std::lock_guard<std::mutex> lock(ctx->active_recorders_mutex);
    for (auto* r : ctx->active_recorders) {
        if (r == rec) return;  /* already registered */
    }
    ctx->active_recorders.push_back(rec);
}

/* Snap all active recorders and INSERT their recordings without submitting.
 * Clears the active set. Callers that need a submit follow with one. */
static void cevg_context_snap_and_insert(CevgContext* ctx) {
    if (!ctx || !ctx->ctx) return;
    std::vector<graphite::Recorder*> recorders;
    {
        std::lock_guard<std::mutex> lock(ctx->active_recorders_mutex);
        recorders.swap(ctx->active_recorders);
    }
    for (auto* rec : recorders) {
        if (auto recording = rec->snap()) {
            graphite::InsertRecordingInfo info;
            info.fRecording = recording.get();
            ctx->ctx->insertRecording(info);
        }
    }
}

/* Snap all active recorders, insert, and submit. Clears the active set. */
static void cevg_context_snap_and_submit(CevgContext* ctx, graphite::SyncToCpu sync) {
    if (!ctx || !ctx->ctx) return;
    cevg_context_snap_and_insert(ctx);
    ctx->ctx->submit(sync);
}

/* ===================================================================
 * Canvas
 * =================================================================== */
extern "C" CevgCanvas* cevg_canvas_create(CevgSurface* surface) {
    return cevg_canvas_create_with_recorder(surface, nullptr);
}

extern "C" CevgCanvas* cevg_canvas_create_with_recorder(CevgSurface* surface,
                                                        CevgRecorder* rec) {
    if (!surface || !surface->surface) return nullptr;

    /* For a window surface, acquire the next swapchain image now so the
     * present at end-of-frame has an image to blit into. */
    if (surface->swapchain) {
#ifdef CEVG_VULKAN
        if (!cevg_swapchain_acquire(surface->swapchain)) {
            return nullptr;  /* OUT_OF_DATE: caller should resize and retry */
        }
#else
        return nullptr;  /* swapchain without Vulkan should not happen */
#endif
    }

    CevgCanvas* c = new (std::nothrow) CevgCanvas_();
    if (!c) {
        /* OOM: release the acquired swapchain image so the next frame can
         * acquire cleanly.  Without this, the next acquire would find the
         * fence already signalled and currentIndex potentially stale. */
#ifdef CEVG_VULKAN
        if (surface->swapchain) surface->swapchain->acquired = false;
#endif
        return nullptr;
    }
    c->canvas   = surface->surface->getCanvas();
    c->surface  = surface;
    c->picRec   = nullptr;

    /* In Graphite a surface can only be recorded through the recorder that
     * created it. The `rec` argument is therefore only meaningful when it
     * matches the surface's owner; a foreign recorder is ignored (with the
     * binding kept correct) rather than silently producing draws that go
     * nowhere. Real parallelism comes from creating the SURFACE with the
     * worker's recorder (cevg_surface_create_with_recorder), not from
     * passing a foreign recorder here.
     *
     * We keep a CevgRecorder* wrapper around the owner for bookkeeping. */
    graphite::Recorder* owner = surface->ownerRecorder;
    if (rec && rec->get() == owner) {
        c->recorder = rec;                                  /* caller's wrapper, matches owner */
    } else {
        c->recorder = cevg_context_default_recorder(surface->ctx);
        /* default wrapper is only valid when owner == default; otherwise we
         * still register the raw owner below, which is what actually matters. */
    }

    /* Register the recorder that the draws ACTUALLY land in — the surface
     * owner — so present/flush snap it. This is the fix for the previous
     * code, which registered the (possibly foreign, possibly default)
     * wrapper recorder rather than the surface's true owner, and could both
     * snap an empty recorder and miss the one holding the draws. */
    cevg_context_register_recorder(surface->ctx, owner);

    return c;
}

extern "C" void cevg_canvas_destroy(CevgCanvas* canvas) {
    if (!canvas) return;
    /* Display-list capture canvases own their picture recorder. */
    delete canvas->picRec;
    delete canvas;
}

extern "C" void cevg_canvas_save(CevgCanvas* c)    { if (c && c->canvas) c->canvas->save(); }
extern "C" void cevg_canvas_restore(CevgCanvas* c) { if (c && c->canvas) c->canvas->restore(); }
extern "C" int  cevg_canvas_get_save_count(const CevgCanvas* c) {
    return (c && c->canvas) ? c->canvas->getSaveCount() : 0;
}

extern "C" void cevg_canvas_translate(CevgCanvas* c, float dx, float dy) {
    if (c && c->canvas) c->canvas->translate(dx, dy);
}
extern "C" void cevg_canvas_scale(CevgCanvas* c, float sx, float sy) {
    if (c && c->canvas) c->canvas->scale(sx, sy);
}
extern "C" void cevg_canvas_rotate(CevgCanvas* c, float deg) {
    if (c && c->canvas) c->canvas->rotate(deg);
}
extern "C" void cevg_canvas_skew(CevgCanvas* c, float kx, float ky) {
    if (c && c->canvas) c->canvas->skew(kx, ky);
}
extern "C" void cevg_canvas_concat(CevgCanvas* c, const float m[9]) {
    if (c && c->canvas && m) c->canvas->concat(ToSkMatrix(m));
}
extern "C" void cevg_canvas_reset_matrix(CevgCanvas* c) {
    if (c && c->canvas) c->canvas->resetMatrix();
}
extern "C" void cevg_canvas_get_transform(const CevgCanvas* c, float m[9]) {
    if (!c || !c->canvas || !m) return;
    SkMatrix sk = c->canvas->getTotalMatrix();
    m[0] = sk.get(SkMatrix::kMScaleX); m[1] = sk.get(SkMatrix::kMSkewY);  m[2] = 0;
    m[3] = sk.get(SkMatrix::kMSkewX);  m[4] = sk.get(SkMatrix::kMScaleY); m[5] = 0;
    m[6] = sk.get(SkMatrix::kMTransX); m[7] = sk.get(SkMatrix::kMTransY); m[8] = 1;
}

extern "C" void cevg_canvas_clip_rect(CevgCanvas* c, float x, float y, float w, float h) {
    if (c && c->canvas) c->canvas->clipRect(SkRect::MakeXYWH(x, y, w, h), true);
}
extern "C" void cevg_canvas_clip_path(CevgCanvas* c, const CevgPath* p) {
    if (c && c->canvas && p) c->canvas->clipPath(CevgPathToSkPath(p), true);
}
extern "C" void cevg_canvas_get_clip_bounds(CevgCanvas* c, float rect[4]) {
    if (!c || !c->canvas || !rect) return;
    SkIRect b = c->canvas->getDeviceClipBounds();
    if (b.isEmpty()) { rect[0]=rect[1]=rect[2]=rect[3]=0; return; }
    rect[0] = (float)b.left(); rect[1] = (float)b.top();
    rect[2] = (float)b.width(); rect[3] = (float)b.height();
}

extern "C" void cevg_canvas_clear(CevgCanvas* c, float r, float g, float b, float a) {
    if (c && c->canvas) c->canvas->clear(SkColor4f{r, g, b, a});
}

extern "C" void cevg_canvas_save_layer(CevgCanvas* c, float alpha,
                                       const float bounds[4], CevgPaint* paint) {
    if (!c || !c->canvas) return;
    SkRect rect;
    const SkRect* pbounds = nullptr;
    if (bounds) { rect = SkRect::MakeXYWH(bounds[0], bounds[1], bounds[2], bounds[3]); pbounds = &rect; }

    SkPaint layerPaint;
    if (paint) {
        layerPaint = CevgPaintToSkPaint(paint, c->surface ? c->surface->ctx : nullptr);
    }
    layerPaint.setAlphaf(layerPaint.getAlphaf() * alpha);

    SkCanvas::SaveLayerRec rec;
    rec.fBounds = pbounds;
    rec.fPaint = &layerPaint;

    /* Backdrop filter. Modern Graphite supports a backdrop in SaveLayerRec
     * directly; build it from the paint's backdrop_* settings.
     * Crop the backdrop output to the layer bounds so the blur/shadow
     * doesn't leak outside its cell (Graphite's fBounds alone doesn't
     * clip the backdrop filter output). */
    sk_sp<SkImageFilter> backdrop;
    if (paint && paint->has_backdrop_blur) {
        backdrop = SkImageFilters::Blur(paint->backdrop_blur_sigma_x,
                                        paint->backdrop_blur_sigma_y,
                                        SkTileMode::kClamp,
                                        /*input=*/nullptr,
                                        /*cropRect=*/pbounds);
    }
    if (paint && paint->has_backdrop_shadow) {
        backdrop = SkImageFilters::DropShadow(
            paint->backdrop_shadow_dx, paint->backdrop_shadow_dy,
            paint->backdrop_shadow_sigma, paint->backdrop_shadow_sigma,
            ArgbToSkColor(paint->backdrop_shadow_color), backdrop,
            /*cropRect=*/pbounds);
    }
    if (backdrop) rec.fBackdrop = backdrop.get();

    c->canvas->saveLayer(rec);
}

extern "C" void cevg_canvas_restore_layer(CevgCanvas* c) {
    if (c && c->canvas) c->canvas->restore();
}

/* ---- Draw primitives ---- */
static CevgContext* cevg_ctx_of(CevgCanvas* c) {
    return c && c->surface ? c->surface->ctx : nullptr;
}

extern "C" void cevg_canvas_draw_paint(CevgCanvas* c, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    c->canvas->drawPaint(CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_rect(CevgCanvas* c, float x, float y, float w, float h, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    c->canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_round_rect(CevgCanvas* c, float x, float y, float w, float h,
                                            float rx, float ry, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    c->canvas->drawRRect(SkRRect::MakeRectXY(SkRect::MakeXYWH(x, y, w, h), rx, ry),
                         CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_circle(CevgCanvas* c, float cx, float cy, float r, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    c->canvas->drawCircle(cx, cy, r, CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_oval(CevgCanvas* c, float cx, float cy, float rx, float ry, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    c->canvas->drawOval(SkRect::MakeXYWH(cx - rx, cy - ry, 2*rx, 2*ry),
                        CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_line(CevgCanvas* c, float x0, float y0, float x1, float y1, CevgPaint* paint) {
    if (!c || !c->canvas) return;
    SkPaint p = CevgPaintToSkPaint(paint, cevg_ctx_of(c));
    p.setStyle(SkPaint::kStroke_Style);  /* a line has no fill */
    c->canvas->drawLine(x0, y0, x1, y1, p);
}
extern "C" void cevg_canvas_draw_path(CevgCanvas* c, const CevgPath* path, CevgPaint* paint) {
    if (!c || !c->canvas || !path) return;
    c->canvas->drawPath(CevgPathToSkPath(path), CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_image(CevgCanvas* c, const CevgImage* image, float x, float y, CevgPaint* paint) {
    if (!c || !c->canvas || !image) return;
    CevgContext* ctx = cevg_ctx_of(c);
    graphite::Recorder* rec = c->surface ? c->surface->ownerRecorder : nullptr;
    sk_sp<SkImage> img = EnsureGraphiteImageRec(image, rec);
    if (!img) return;
    SkPaint p = CevgPaintToSkPaint(paint, ctx);
    SkSamplingOptions samp = ToSkSampling(paint ? paint->filter_quality : kCevgFilterQuality_Linear);
    c->canvas->drawImage(img, x, y, samp, &p);
}
extern "C" void cevg_canvas_draw_image_rect(CevgCanvas* c, const CevgImage* image,
                                            const float src_rect[4], const float dst_rect[4], CevgPaint* paint) {
    if (!c || !c->canvas || !image || !src_rect || !dst_rect) return;
    CevgContext* ctx = cevg_ctx_of(c);
    graphite::Recorder* rec = c->surface ? c->surface->ownerRecorder : nullptr;
    sk_sp<SkImage> img = EnsureGraphiteImageRec(image, rec);
    if (!img) return;
    SkPaint p = CevgPaintToSkPaint(paint, ctx);
    SkRect src = SkRect::MakeXYWH(src_rect[0], src_rect[1], src_rect[2], src_rect[3]);
    SkRect dst = SkRect::MakeXYWH(dst_rect[0], dst_rect[1], dst_rect[2], dst_rect[3]);
    SkSamplingOptions samp = ToSkSampling(paint ? paint->filter_quality : kCevgFilterQuality_Linear);
    c->canvas->drawImageRect(img, src, dst, samp, &p, SkCanvas::kStrict_SrcRectConstraint);
}
extern "C" void cevg_canvas_draw_image_nine(CevgCanvas* c, const CevgImage* image,
                                            const float center[4], const float dst[4], CevgPaint* paint) {
    if (!c || !c->canvas || !image || !center || !dst) return;
    CevgContext* ctx = cevg_ctx_of(c);
    graphite::Recorder* rec = c->surface ? c->surface->ownerRecorder : nullptr;
    sk_sp<SkImage> img = EnsureGraphiteImageRec(image, rec);
    if (!img) return;
    SkPaint p = CevgPaintToSkPaint(paint, ctx);
    SkSamplingOptions samp = ToSkSampling(paint ? paint->filter_quality
                                                : kCevgFilterQuality_Linear);

    /* drawImageNine() is a no-op on the Graphite/Vulkan backend, so the
     * nine-patch is decomposed manually into up to 9 drawImageRect calls:
     * 4 unscaled corners, 4 single-axis-stretched edges, 1 doubly-stretched
     * center. center is LTRB in image pixels; dst is XYWH in user space. */
    const float iw = (float)image->width, ih = (float)image->height;
    float sl = center[0], st = center[1], sr = center[2], sb = center[3];
    if (sl < 0) sl = 0; if (st < 0) st = 0;
    if (sr > iw) sr = iw; if (sb > ih) sb = ih;
    if (sr < sl) sr = sl; if (sb < st) sb = st;

    const float dx = dst[0], dy = dst[1], dw = dst[2], dh = dst[3];
    /* Source column/row widths. */
    const float sLw = sl, sCw = sr - sl, sRw = iw - sr;
    const float sTh = st, sCh = sb - st, sBh = ih - sb;
    /* Destination fixed corner sizes (corners unscaled => same px size). */
    const float dLw = sLw, dRw = sRw, dTh = sTh, dBh = sBh;
    const float dCw = dw - dLw - dRw;   /* stretched horizontally */
    const float dCh = dh - dTh - dBh;   /* stretched vertically   */

    auto piece = [&](float sx, float sy, float sw, float sh,
                     float ddx, float ddy, float ddw, float ddh) {
        if (sw <= 0 || sh <= 0 || ddw <= 0 || ddh <= 0) return;
        SkRect s = SkRect::MakeXYWH(sx, sy, sw, sh);
        SkRect d = SkRect::MakeXYWH(ddx, ddy, ddw, ddh);
        c->canvas->drawImageRect(img, s, d, samp, &p, SkCanvas::kStrict_SrcRectConstraint);
    };

    const float dxC = dx + dLw, dxR = dx + dw - dRw;
    const float dyC = dy + dTh, dyB = dy + dh - dBh;
    const float sxC = sl, sxR = sr, syC = st, syB = sb;

    /* corners */
    piece(0,   0,   sLw, sTh, dx,  dy,  dLw, dTh);   /* TL */
    piece(sxR, 0,   sRw, sTh, dxR, dy,  dRw, dTh);   /* TR */
    piece(0,   syB, sLw, sBh, dx,  dyB, dLw, dBh);   /* BL */
    piece(sxR, syB, sRw, sBh, dxR, dyB, dRw, dBh);   /* BR */
    /* edges */
    piece(sxC, 0,   sCw, sTh, dxC, dy,  dCw, dTh);   /* top    */
    piece(sxC, syB, sCw, sBh, dxC, dyB, dCw, dBh);   /* bottom */
    piece(0,   syC, sLw, sCh, dx,  dyC, dLw, dCh);   /* left   */
    piece(sxR, syC, sRw, sCh, dxR, dyC, dRw, dCh);   /* right  */
    /* center */
    piece(sxC, syC, sCw, sCh, dxC, dyC, dCw, dCh);
}
extern "C" void cevg_canvas_draw_text_blob(CevgCanvas* c, const CevgTextBlob* blob,
                                           float x, float y, CevgPaint* paint) {
    if (!c || !c->canvas || !blob) return;

    /* SkShaper's font-iterator (with the fallback font manager built in
     * cevg_text_blob_make_ex) resolves all fallback glyphs into native_blob,
     * so a single drawTextBlob renders the whole run with correct per-glyph
     * fonts. native_blob is always present when the blob has glyphs; nothing
     * to draw if it isn't. */
    if (!blob->native_blob) return;
    c->canvas->drawTextBlob(blob->native_blob.get(), x, y,
                            CevgPaintToSkPaint(paint, cevg_ctx_of(c)));
}
extern "C" void cevg_canvas_draw_surface(CevgCanvas* c, const CevgSurface* surface,
                                         float x, float y, CevgPaint* paint) {
    if (!c || !c->canvas || !surface || !surface->surface) return;
    sk_sp<SkImage> snap = surface->surface->makeImageSnapshot();
    if (!snap) return;
    SkPaint p = CevgPaintToSkPaint(paint, cevg_ctx_of(c));
    c->canvas->drawImage(snap, x, y, SkSamplingOptions(SkFilterMode::kLinear), &p);
}

extern "C" void cevg_canvas_flush(CevgCanvas* c) {
    if (!c || !c->surface) return;
    CevgContext* ctx = c->surface->ctx;
    if (!ctx || !ctx->ctx) return;
    cevg_context_snap_and_submit(ctx, graphite::SyncToCpu::kNo);
    /* snap() consumed every active recorder's pending recording and cleared
     * the active set. Re-register THIS canvas' owner recorder so that draws
     * issued after the flush, on this canvas, are snapped again at the next
     * flush/present. Without this, a draw → flush → draw → present sequence
     * would silently drop the post-flush draws. */
    cevg_context_register_recorder(ctx, c->surface->ownerRecorder);
}

/* ===================================================================
 * Display list (SkPicture-backed)
 * =================================================================== */
extern "C" CevgCanvas* cevg_display_list_record_begin(CevgContext* ctx, int w, int h) {
    if (!ctx || w <= 0 || h <= 0) return nullptr;
    CevgCanvas* c = new (std::nothrow) CevgCanvas_();
    if (!c) return nullptr;
    c->picRec = new SkPictureRecorder();
    c->canvas = c->picRec->beginRecording(SkRect::MakeWH((float)w, (float)h));
    c->surface = nullptr;
    c->recorder = nullptr;
    c->dlWidth = w;
    c->dlHeight = h;
    return c;
}

extern "C" CevgDisplayList* cevg_display_list_record_end(CevgCanvas* capture_canvas) {
    if (!capture_canvas || !capture_canvas->picRec) return nullptr;
    CevgDisplayList* dl = new (std::nothrow) CevgDisplayList_();
    if (!dl) return nullptr;
    dl->picture = capture_canvas->picRec->finishRecordingAsPicture();
    dl->width  = capture_canvas->dlWidth;
    dl->height = capture_canvas->dlHeight;
    delete capture_canvas->picRec;
    delete capture_canvas;
    return dl;
}

extern "C" void cevg_display_list_destroy(CevgDisplayList* dl) {
    if (!dl) return;
    dl->picture.reset();
    delete dl;
}

extern "C" void cevg_canvas_draw_display_list(CevgCanvas* c, const CevgDisplayList* dl,
                                              const float matrix[9]) {
    if (!c || !c->canvas || !dl || !dl->picture) return;
    if (matrix) {
        SkMatrix m = ToSkMatrix(matrix);
        c->canvas->drawPicture(dl->picture.get(), &m, nullptr);
    } else {
        c->canvas->drawPicture(dl->picture.get());
    }
}

/* ===================================================================
 * Shared font manager for typeface/blob creation
 * -------------------------------------------------------------------
 * Typeface and TextBlob creation take no CevgContext, so they use a
 * process-wide font manager created lazily (thread-safe). It uses the
 * same portable selection as the per-context font manager.
 * =================================================================== */
static sk_sp<SkFontMgr> cevg_shared_fontmgr() {
    static std::once_flag once;
    static sk_sp<SkFontMgr> mgr;
    std::call_once(once, [](){ mgr = cevg_make_fontmgr(); });
    return mgr;
}

/* ===================================================================
 * Typeface / TextBlob / Image  (shaping logic preserved from the
 * previous backend; adapted to the shared font manager and extern "C")
 * =================================================================== */
extern "C" CevgTypeface* cevg_typeface_create_from_file(const char* path, int ttc_index) {
    if (!path) return nullptr;
    auto fontMgr = cevg_shared_fontmgr();
    if (!fontMgr) return nullptr;
    auto tf = fontMgr->makeFromFile(path, ttc_index);
    if (!tf) return nullptr;

    CevgTypeface* ntf = new (std::nothrow) CevgTypeface();
    if (!ntf) return nullptr;
    ntf->typeface = std::move(tf);
    ntf->ref_count = 1;
    return ntf;
}

extern "C" CevgTypeface* cevg_typeface_create_from_data(const void* data, size_t len, int ttc_index) {
    if (!data || !len) return nullptr;
    auto fontMgr = cevg_shared_fontmgr();
    if (!fontMgr) return nullptr;
    auto skData = SkData::MakeWithCopy(data, len);
    auto tf = fontMgr->makeFromData(skData, ttc_index);
    if (!tf) return nullptr;

    CevgTypeface* ntf = new (std::nothrow) CevgTypeface();
    if (!ntf) return nullptr;
    ntf->typeface = std::move(tf);
    ntf->ref_count = 1;
    return ntf;
}

extern "C" void cevg_typeface_ref(CevgTypeface* typeface) {
    if (!typeface) return;
    typeface->ref_count.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void cevg_typeface_unref(CevgTypeface* typeface) {
    if (!typeface) return;
    /* Release on the decrement, acquire fence before destruction, so the
     * thread that frees the object sees all prior writes from other threads. */
    if (typeface->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete typeface;
    }
}

extern "C" void cevg_typeface_get_metrics(const CevgTypeface* typeface,
                               float font_size, CevgFontMetrics* metrics) {
    if (!typeface || !metrics) return;
    SkFont font(typeface->typeface, font_size);
    SkFontMetrics fm;
    font.getMetrics(&fm);
    /* Write each field only if the caller's struct_size reaches it, so an
     * older caller with a smaller CevgFontMetrics is never overrun. The
     * caller is responsible for setting struct_size; if it left it zero we
     * treat that as "no fields available" and write nothing. */
    uint32_t sz = metrics->struct_size;
    if (CEVG_HAS_FIELD(CevgFontMetrics, sz, ascent))     metrics->ascent     = fm.fAscent;
    if (CEVG_HAS_FIELD(CevgFontMetrics, sz, descent))    metrics->descent    = fm.fDescent;
    if (CEVG_HAS_FIELD(CevgFontMetrics, sz, leading))    metrics->leading    = fm.fLeading;
    if (CEVG_HAS_FIELD(CevgFontMetrics, sz, cap_height)) metrics->cap_height = fm.fCapHeight;
    if (CEVG_HAS_FIELD(CevgFontMetrics, sz, x_height))   metrics->x_height   = fm.fXHeight;
}

/* ===================================================================
 * Custom SkShaper RunHandler — collects glyphs, clusters, and BiDi runs
 * -------------------------------------------------------------------
 * SkTextBlobBuilderRunHandler produces a correct SkTextBlob but does not
 * expose per-glyph cluster indices or per-run BiDi direction. This custom
 * handler collects ALL shaping metadata so that CevgTextBlob carries real
 * cluster→byte-offset mappings and multiple visual-order BiDi runs.
 * =================================================================== */
struct CevgShaperResult {
    /* Per-glyph data, indexed in visual order. */
    std::vector<SkGlyphID>  glyphIDs;
    std::vector<SkPoint>    positions;
    std::vector<uint32_t>   clusters;   /* UTF-8 byte offset for each glyph */
    std::vector<float>      advances;   /* per-glyph advance width from font metrics */

    /* Per-run data, in visual order. */
    struct Run {
        int         glyphStart;     /* index into glyphIDs[] */
        int         glyphCount;
        int         byteStart;      /* UTF-8 byte offset of the run's first char */
        int         byteEnd;        /* UTF-8 byte offset one past the run's last char */
        CevgTextDirection dir;
        float       fontSize;
    };
    std::vector<Run> runs;

    /* The shaped SkTextBlob (for efficient drawing). */
    sk_sp<SkTextBlob> nativeBlob;

    /* Typographic metrics from the primary font. */
    float lineHeight = 0;
    float shapedWidth = 0;   /* rightmost pen-advance edge (max of origin+advance) */

    /* Resolved paragraph base direction (the BiDi base level used for shaping),
     * LTR or RTL. Surfaced through the blob as para_dir. */
    CevgTextDirection baseDir = kCevgDir_LTR;
};

class CevgRunHandler : public SkShaper::RunHandler {
    const char*        fText;
    size_t             fTextLen;
    CevgShaperResult*  fResult;
    int                fGlyphOffset;  /* running glyph count across runs */
    SkShaper::RunHandler::Buffer fCurrentBuffer;  /* buffer from runBuffer() */
    /* Also build a native SkTextBlob in the same pass, eliminating the
     * need for a second shaping pass. */
    SkTextBlobBuilderRunHandler fBlobHandler;
public:
    CevgRunHandler(const char* text, size_t len, CevgShaperResult* result,
                   SkPoint blobOffset = {0, 0})
        : fText(text), fTextLen(len), fResult(result), fGlyphOffset(0),
          fBlobHandler(text, blobOffset) {}

    sk_sp<SkTextBlob> makeBlob() { return fBlobHandler.makeBlob(); }

private:
    void commitRunBuffer(const RunInfo& info) override {
        CevgShaperResult::Run run;
        run.glyphStart = fGlyphOffset;
        run.glyphCount = info.glyphCount;
        run.byteStart  = (int)info.utf8Range.begin();
        run.byteEnd    = (int)(info.utf8Range.begin() + info.utf8Range.size());
        run.dir        = (info.fBidiLevel & 1) ? kCevgDir_RTL : kCevgDir_LTR;
        run.fontSize   = info.fFont.getSize();

        for (int i = 0; i < info.glyphCount; i++) {
            fResult->glyphIDs.push_back(fCurrentBuffer.glyphs[i]);
            fResult->positions.push_back(fCurrentBuffer.positions[i]);
            fResult->clusters.push_back(
                fCurrentBuffer.clusters ? fCurrentBuffer.clusters[i]
                                        : (uint32_t)run.byteStart);
        }

        /* Get real per-glyph advances from this run's font.
         * SkFont::getWidths returns the nominal advance for each glyph,
         * which is the correct "cell width" — positions already include
         * kerning/GPOS adjustments, but advance is the typographic step. */
        if (info.glyphCount > 0) {
            std::vector<SkScalar> adv(info.glyphCount);
            info.fFont.getWidths(SkSpan<const SkGlyphID>{fCurrentBuffer.glyphs, (size_t)info.glyphCount},
                                 SkSpan<SkScalar>{adv.data(), (size_t)info.glyphCount});
            for (int i = 0; i < info.glyphCount; ++i)
                fResult->advances.push_back(adv[i]);
        }

        fResult->runs.push_back(run);
        fGlyphOffset += info.glyphCount;

        /* Forward to the blob handler so it finalizes the native SkTextBlob. */
        fBlobHandler.commitRunBuffer(info);
    }

    Buffer runBuffer(const RunInfo& info) override {
        /* Use the blob handler's buffer directly — the shaper writes into it,
         * and the blob handler builds the SkTextBlob from the same data.
         * We save a copy of the buffer pointer so commitRunBuffer can read
         * the filled-in glyphs/positions for our metadata collection. */
        Buffer buf = fBlobHandler.runBuffer(info);
        fCurrentBuffer = buf;
        return buf;
    }

    void commitRunInfo() override { fBlobHandler.commitRunInfo(); }
    void beginLine() override { fBlobHandler.beginLine(); }
    void commitLine() override { fBlobHandler.commitLine(); }
    void runInfo(const RunInfo& info) override { fBlobHandler.runInfo(info); }
};

/* Shape text and fill a CevgShaperResult with full metadata. */
static bool cevg_shape_text(const char* text, size_t len,
                            const SkFont& font, sk_sp<SkFontMgr> fontMgr,
                            CevgTextDirection dir, CevgShaperResult* result) {
    std::string txt(text, len);

    bool leftToRight;
    if (dir == kCevgDir_Auto) {
        leftToRight = true;
        for (size_t i = 0; i < txt.size(); ) {
            uint32_t cp = (unsigned char)txt[i];
            int cplen = 1;
            if (cp >= 0xF0 && i+3 < txt.size())      { cp = ((cp&0x07)<<18)|((txt[i+1]&0x3F)<<12)|((txt[i+2]&0x3F)<<6)|(txt[i+3]&0x3F); cplen=4; }
            else if (cp >= 0xE0 && i+2 < txt.size())  { cp = ((cp&0x0F)<<12)|((txt[i+1]&0x3F)<<6)|(txt[i+2]&0x3F); cplen=3; }
            else if (cp >= 0xC0 && i+1 < txt.size())  { cp = ((cp&0x1F)<<6)|(txt[i+1]&0x3F); cplen=2; }
            if ((cp >= 0x0590 && cp <= 0x08FF) || (cp >= 0xFB50 && cp <= 0xFDFF) ||
                (cp >= 0xFE70 && cp <= 0xFEFF) || (cp >= 0x10800 && cp <= 0x10FFF)) {
                leftToRight = false; break;
            }
            if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') ||
                (cp >= 0x0370 && cp <= 0x03FF) || (cp >= 0x4E00 && cp <= 0x9FFF) ||
                (cp >= 0x0400 && cp <= 0x04FF)) {
                leftToRight = true; break;
            }
            i += cplen;
        }
    } else {
        leftToRight = (dir == kCevgDir_LTR);
    }
    /* Remember the resolved base direction so the blob reports it and so paraDir
     * is taken from the BiDi base, not from the first visual run. */
    result->baseDir = leftToRight ? kCevgDir_LTR : kCevgDir_RTL;

    auto shaper = SkShaper::Make(fontMgr);
    if (!shaper) shaper = SkShaper::MakePrimitive();
    if (!shaper) return false;

    /* Get font metrics BEFORE shaping so we can pass the ascent as the
     * blob handler offset. SkTextBlobBuilderRunHandler::commitRunInfo()
     * does fCurrentPosition.fY -= fMaxRunAscent, which shifts glyph Y
     * by |ascent|. By passing offset {0, ascent} (ascent is negative),
     * the shift cancels out: fCurrentPosition.fY = ascent - fMaxRunAscent
     * ≈ 0, so the blob's internal coordinates are baseline-relative.
     * This makes drawTextBlob(blob, x, y) treat y as baseline (Skia
     * convention) without any runtime compensation. */
    SkFontMetrics fm;
    font.getMetrics(&fm);

    CevgRunHandler handler(txt.c_str(), txt.size(), result, {0, fm.fAscent});
    auto bidiIter = SkShaper::MakeBiDiRunIterator(txt.c_str(), txt.size(), leftToRight ? 0 : 1);
    auto scriptIter = SkShaper::MakeScriptRunIterator(txt.c_str(), txt.size(), SkSetFourByteTag(0, 0, 0, 0));
    auto fontIter = SkShaper::MakeFontMgrRunIterator(txt.c_str(), txt.size(), font, fontMgr);

    if (!fontIter) {
        fontIter = std::make_unique<SkShaper::TrivialFontRunIterator>(font, txt.size());
    }

    if (bidiIter && scriptIter && fontIter) {
        SkShaper::TrivialLanguageRunIterator langIter("en", txt.size());
        shaper->shape(txt.c_str(), txt.size(),
                     *fontIter, *bidiIter, *scriptIter, langIter,
                     nullptr, 0,
                     SK_ScalarMax,
                     &handler);
    }

    /* The native SkTextBlob was built in the same pass by the handler's
     * internal SkTextBlobBuilderRunHandler — no second shaping needed. */
    result->nativeBlob = handler.makeBlob();

    /* Typographic metrics (fm was already fetched above for the blob offset). */
    result->lineHeight = fm.fDescent - fm.fAscent;

    /* Compute shaped width as the rightmost pen-advance edge.
     * Layout width = max(glyph_origin + its_advance) across all glyphs.
     * This is the true typographic advance width — the position where the
     * pen would rest after the last glyph. Using bounds().right() was wrong
     * because SkTextBlob::bounds() is a conservative ink bounding box that
     * often exceeds the actual advance width. */
    float w = 0.0f;
    for (size_t i = 0; i < result->positions.size(); ++i) {
        float adv = (i < result->advances.size()) ? result->advances[i] : 0.0f;
        w = std::max(w, result->positions[i].x() + adv);
    }
    result->shapedWidth = w;

    return result->glyphIDs.size() > 0;
}

/* Fill a CevgTextBlob from a CevgShaperResult. */
static CevgTextBlob* cevg_blob_from_result(CevgShaperResult* result, float fontSize,
                                           CevgTextDirection paraDir,
                                           const CevgTypeface* primaryTypeface,
                                           size_t textLen) {
    int totalGlyphs = (int)result->glyphIDs.size();
    if (totalGlyphs <= 0) return nullptr;

    CevgTextBlob* blob = new (std::nothrow) CevgTextBlob();
    if (!blob) return nullptr;
    memset(blob, 0, sizeof(CevgTextBlob));

    blob->glyph_count     = totalGlyphs;
    blob->glyph_ids       = (uint16_t*)calloc(totalGlyphs, sizeof(uint16_t));
    blob->positions_x     = (float*)calloc(totalGlyphs, sizeof(float));
    blob->positions_y     = (float*)calloc(totalGlyphs, sizeof(float));
    blob->cluster_indices = (int*)calloc(totalGlyphs, sizeof(int));

    if (!blob->glyph_ids || !blob->positions_x || !blob->positions_y || !blob->cluster_indices) {
        free(blob->glyph_ids); free(blob->positions_x);
        free(blob->positions_y); free(blob->cluster_indices);
        delete blob; return nullptr;
    }

    for (int i = 0; i < totalGlyphs; i++) {
        blob->glyph_ids[i]       = result->glyphIDs[i];
        blob->positions_x[i]     = result->positions[i].x();
        blob->positions_y[i]     = result->positions[i].y();
        blob->cluster_indices[i] = (int)result->clusters[i];
    }

    blob->height = result->lineHeight;
    blob->width  = result->shapedWidth;
    blob->text_len = textLen;
    blob->native_blob = result->nativeBlob;
    blob->font_size = fontSize;
    blob->para_dir  = paraDir;

    /* Copy real per-glyph advances from the shaper (taken via SkFont::getWidths
     * per run). These are the true typographic advance widths, not reconstructed
     * from position differences. */
    blob->advances = (float*)calloc(totalGlyphs, sizeof(float));
    if (blob->advances && (int)result->advances.size() == totalGlyphs) {
        for (int i = 0; i < totalGlyphs; i++)
            blob->advances[i] = result->advances[i];
    }

    /* Store real run info from the shaper. */
    blob->run_count = (int)result->runs.size();
    if (blob->run_count > 0) {
        blob->runs = (CevgTextRun*)calloc(blob->run_count, sizeof(CevgTextRun));
        if (blob->runs) {
            for (int i = 0; i < blob->run_count; i++) {
                blob->runs[i].glyph_start = result->runs[i].glyphStart;
                blob->runs[i].glyph_count = result->runs[i].glyphCount;
                blob->runs[i].byte_start  = result->runs[i].byteStart;
                blob->runs[i].byte_end    = result->runs[i].byteEnd;
                blob->runs[i].dir         = result->runs[i].dir;
            }
        }
    }

    /* Store primary typeface ref. */
    if (primaryTypeface) {
        blob->primary_face = (CevgTypeface*)primaryTypeface;
        cevg_typeface_ref((CevgTypeface*)primaryTypeface);
    }

    return blob;
}

/* ===================================================================
 * TextBlob
 * =================================================================== */
/* ================================================================
 * Glyph Y positions in CevgTextBlob (baseline convention)
 * ================================================================
 *
 * SkTextBlobBuilderRunHandler::commitRunInfo() shifts glyph Y by
 * -fMaxRunAscent (i.e. +|ascent|), which makes the handler's offset
 * point represent the TOP of the text, not the baseline.
 *
 * To preserve Skia's baseline convention in our public API, we pass
 * {0, ascent} (ascent is negative) as the handler offset. This way
 * commitRunInfo()'s shift cancels out:
 *   fCurrentPosition.fY = ascent - fMaxRunAscent ≈ 0
 * so the blob's internal coordinates are baseline-relative, and
 * drawTextBlob(blob, x, y) treats y as baseline — no runtime
 * compensation needed.
 * ================================================================
 */
extern "C" CevgTextBlob* cevg_text_blob_make(const char* text, size_t len,
                                  const CevgTypeface* typeface, float size,
                                  CevgTextDirection dir) {
    if (!text || !len || !typeface) return nullptr;

    SkFont font(typeface->typeface, size);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setHinting(SkFontHinting::kNormal);

    CevgShaperResult result;
    auto fontMgr = cevg_shared_fontmgr();
    if (!cevg_shape_text(text, len, font, fontMgr, dir, &result)) return nullptr;

    /* Paragraph base direction = the shaper's resolved base (first strong char for
     * Auto), NOT the first VISUAL run: for an RTL paragraph the first visual run
     * is the logically-last run and is frequently LTR, which would mislabel the
     * whole paragraph and break end-of-line cursor placement / alignment. */
    CevgTextDirection paraDir = result.baseDir;

    return cevg_blob_from_result(&result, size, paraDir, typeface, len);
}

/* ---- UTF-8 decode helper ---- */
static uint32_t utf8_decode(const char* s, int len, int* out_len) {
    const unsigned char* p = (const unsigned char*)s;
    if (len <= 0) { *out_len = 0; return 0; }
    uint32_t cp = 0;
    if (p[0] < 0x80) { cp = p[0]; *out_len = 1; }
    else if ((p[0] & 0xE0) == 0xC0 && len >= 2) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); *out_len = 2;
    } else if ((p[0] & 0xF0) == 0xE0 && len >= 3) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); *out_len = 3;
    } else if ((p[0] & 0xF8) == 0xF0 && len >= 4) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) |
             ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); *out_len = 4;
    } else { *out_len = 1; cp = p[0]; }
    return cp;
}

extern "C" CevgTextBlob* cevg_text_blob_make_ex(const char* text, size_t len,
                                     const CevgTypeface* typeface, float size,
                                     CevgTextDirection dir,
                                     const CevgTypeface** fallbacks,
                                     int fallback_count) {
    if (!text || !len || !typeface) return nullptr;

    /* If no fallbacks requested, delegate to the simple version */
    if (!fallbacks || fallback_count <= 0) {
        return cevg_text_blob_make(text, len, typeface, size, dir);
    }

    /* Build a custom SkFontMgr that tries the caller's fallback typefaces
     * before falling back to the system font manager.  This lets SkShaper's
     * font-iterator handle the entire shaping pipeline correctly (kerning,
     * ligatures, BiDi reordering) instead of the old per-glyph reshaping
     * approach which broke kerning and ligatures. */
    auto systemMgr = cevg_shared_fontmgr();

    /* Collect the fallback SkTypefaces into a vector for the custom mgr. */
    std::vector<sk_sp<SkTypeface>> fbTypefaces;
    fbTypefaces.reserve(fallback_count);
    for (int i = 0; i < fallback_count; i++) {
        if (fallbacks[i] && fallbacks[i]->typeface) {
            fbTypefaces.push_back(fallbacks[i]->typeface);
        }
    }

    /* Custom font manager: tries caller's fallbacks first, then system. */
    class FallbackFontMgr : public SkFontMgr {
        std::vector<sk_sp<SkTypeface>> fFallbacks;
        sk_sp<SkFontMgr> fSystem;
    public:
        FallbackFontMgr(std::vector<sk_sp<SkTypeface>>&& fallbacks, sk_sp<SkFontMgr> system)
            : fFallbacks(std::move(fallbacks)), fSystem(std::move(system)) {}

        int onCountFamilies() const override { return fSystem->countFamilies(); }
        void onGetFamilyName(int index, SkString* familyName) const override {
            fSystem->getFamilyName(index, familyName);
        }
        sk_sp<SkFontStyleSet> onCreateStyleSet(int which) const override {
            return fSystem->createStyleSet(which);
        }
        sk_sp<SkFontStyleSet> onMatchFamily(const char familyName[]) const override {
            return fSystem->matchFamily(familyName);
        }
        sk_sp<SkTypeface> onMatchFamilyStyle(const char familyName[],
                                               const SkFontStyle& style) const override {
            return fSystem->matchFamilyStyle(familyName, style);
        }
        sk_sp<SkTypeface> onMatchFamilyStyleCharacter(const char familyName[],
                                                        const SkFontStyle& style,
                                                        const char* bcp47[], int bcp47Count,
                                                        SkUnichar uni) const override {
            for (const auto& tf : fFallbacks) {
                if (tf && tf->unicharToGlyph(uni) != 0) return tf;
            }
            return fSystem->matchFamilyStyleCharacter(familyName, style, bcp47, bcp47Count, uni);
        }
        sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData>, int) const override { return nullptr; }
        sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset>, int) const override { return nullptr; }
        sk_sp<SkTypeface> onMakeFromStreamArgs(std::unique_ptr<SkStreamAsset>, const SkFontArguments&) const override { return nullptr; }
        sk_sp<SkTypeface> onMakeFromFile(const char[], int) const override { return nullptr; }
        sk_sp<SkTypeface> onLegacyMakeTypeface(const char[], SkFontStyle) const override { return nullptr; }
    };

    auto compositeMgr = sk_sp<SkFontMgr>(new FallbackFontMgr(std::move(fbTypefaces), systemMgr));

    SkFont font(typeface->typeface, size);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setHinting(SkFontHinting::kNormal);

    CevgShaperResult result;
    if (!cevg_shape_text(text, len, font, compositeMgr, dir, &result)) return nullptr;

    /* Paragraph base direction = the shaper's resolved base (first strong char for
     * Auto), NOT the first VISUAL run: for an RTL paragraph the first visual run
     * is the logically-last run and is frequently LTR, which would mislabel the
     * whole paragraph and break end-of-line cursor placement / alignment. */
    CevgTextDirection paraDir = result.baseDir;

    return cevg_blob_from_result(&result, size, paraDir, typeface, len);
}

extern "C" void cevg_text_blob_destroy(CevgTextBlob* blob) {
    if (!blob) return;
    if (blob->native_blob) blob->native_blob.reset();
    if (blob->primary_face) cevg_typeface_unref(blob->primary_face);
    free(blob->glyph_ids);
    free(blob->positions_x);
    free(blob->positions_y);
    free(blob->cluster_indices);
    free(blob->advances);
    free(blob->runs);
    delete blob;
}

extern "C" float cevg_text_blob_get_width(const CevgTextBlob* blob) {
    return blob ? blob->width : 0.0f;
}

extern "C" float cevg_text_blob_get_height(const CevgTextBlob* blob) {
    return blob ? blob->height : 0.0f;
}

extern "C" int cevg_text_blob_get_glyph_count(const CevgTextBlob* blob) {
    return blob ? blob->glyph_count : 0;
}

extern "C" void cevg_text_blob_get_glyph_positions(const CevgTextBlob* blob,
                                        float* out_x, float* out_y) {
    if (!blob || !out_x || !out_y) return;
    memcpy(out_x, blob->positions_x, blob->glyph_count * sizeof(float));
    memcpy(out_y, blob->positions_y, blob->glyph_count * sizeof(float));
}

extern "C" void cevg_text_blob_get_cluster_info(const CevgTextBlob* blob,
                                     int* char_indices) {
    if (!blob || !char_indices) return;
    memcpy(char_indices, blob->cluster_indices, blob->glyph_count * sizeof(int));
}

/* Cluster end (one-past-the-last byte) of the cluster that STARTS at byte `cs`,
 * within run `r`: the smallest cluster-start strictly greater than cs among the
 * run's glyphs, or the run's byte_end if none. Relies only on cluster-start byte
 * values (not on visual glyph order), so it is correct for LTR runs, RTL runs,
 * and ligatures (one glyph spanning several bytes). */
static int cevg_cluster_end_in_run(const CevgTextBlob* blob, int r, int cs) {
    const CevgTextRun& run = blob->runs[r];
    int ge = run.glyph_start + run.glyph_count;
    int end = run.byte_end;
    for (int g = run.glyph_start; g < ge; g++) {
        int c = blob->cluster_indices[g];
        if (c > cs && c < end) end = c;
    }
    return end;
}

extern "C" int cevg_text_blob_hit_test(const CevgTextBlob* blob, float x, float y) {
    (void)y;
    if (!blob) return -1;
    if (blob->glyph_count == 0) return -1;

    /* Direction-aware hit testing.
     *
     * The previous version walked glyphs in VISUAL order and, for a click on a
     * glyph's right half, returned cluster_indices[i+1] -- the VISUALLY-next
     * glyph's cluster. That is only correct for LTR runs. In an RTL run the
     * visually-next glyph is the LOGICALLY-PREVIOUS cluster, so the returned byte
     * was off by one cluster throughout RTL text, and disagreed with the
     * cluster/advance data this same blob exposes (clicking where the caret is
     * drawn landed one character away).
     *
     * Treat every glyph as contributing two caret "slots":
     *     leading edge  -> cluster START byte (cluster_indices[i])
     *     trailing edge -> cluster END byte   (next cluster boundary)
     * For an LTR glyph the leading edge is its left edge and the trailing edge
     * its right edge; for an RTL glyph they are mirrored (leading = right edge).
     * Return the byte of whichever slot is nearest x. This is symmetric, agrees
     * with caret positions computed from the same cluster/advance data (so
     * caret<->hit round-trips), and handles ligatures (snaps to the nearest
     * cluster edge) and the end-of-line cursor (the trailing edge of the
     * logically-last glyph is text_len) uniformly, for both LTR and RTL.
     *
     * Note: at an LTR<->RTL boundary one visual x maps to two logical bytes
     * (e.g. the end of an RTL run and the start of the next LTR run coincide).
     * A single-valued hit test must pick one; we pick the earlier run's byte.
     * Callers that need the other side disambiguate with cursor affinity. */
    const int has_runs = (blob->run_count > 0 && blob->runs);

    int   best_byte = 0;
    float best_dist = 3.4e38f;
    auto consider = [&](float edge_x, int byte) {
        float d = x - edge_x; if (d < 0.0f) d = -d;
        if (d < best_dist) { best_dist = d; best_byte = byte; }
    };

    if (!has_runs) {
        /* No BiDi run info: treat as a single LTR run (visual == logical order). */
        for (int i = 0; i < blob->glyph_count; i++) {
            float origin = blob->positions_x[i];
            float adv = blob->advances ? blob->advances[i] : 0.0f;
            int cs = blob->cluster_indices[i];
            int ce = (i + 1 < blob->glyph_count) ? blob->cluster_indices[i + 1]
                                                 : (int)blob->text_len;
            consider(origin,       cs);   /* leading  (left)  */
            consider(origin + adv, ce);   /* trailing (right) */
        }
    } else {
        for (int r = 0; r < blob->run_count; r++) {
            const CevgTextRun& run = blob->runs[r];
            const bool rtl = (run.dir == kCevgDir_RTL);
            int gs = run.glyph_start, ge = gs + run.glyph_count;
            for (int i = gs; i < ge; i++) {
                float origin = blob->positions_x[i];
                float adv = blob->advances ? blob->advances[i] : 0.0f;
                int cs = blob->cluster_indices[i];
                int ce = cevg_cluster_end_in_run(blob, r, cs);
                float lead_x  = rtl ? (origin + adv) : origin;          /* cluster START side */
                float trail_x = rtl ? origin         : (origin + adv);  /* cluster END   side */
                consider(lead_x,  cs);
                consider(trail_x, ce);
            }
        }
    }
    return best_byte;
}

/* Resolved paragraph base direction (LTR/RTL) used by the shaper. For Auto this
 * is decided by the first strong character, not by the first visual run. Useful
 * for text alignment and for placing the cursor at string ends in BiDi text. */
extern "C" CevgTextDirection cevg_text_blob_get_paragraph_direction(const CevgTextBlob* blob) {
    if (!blob) return kCevgDir_LTR;
    return blob->para_dir;
}

/* ---- New text-editor APIs ---- */

extern "C" void cevg_text_blob_get_glyph_advances(const CevgTextBlob* blob, float* out) {
    if (!blob || !out) return;
    if (blob->advances) {
        memcpy(out, blob->advances, blob->glyph_count * sizeof(float));
    }
    /* If advances is NULL (shouldn't happen with current implementation),
     * out is left as-is — caller should zero-initialize for safety. */
}

extern "C" void cevg_text_blob_get_ink_bounds(const CevgTextBlob* blob, float out[4]) {
    if (!blob || !out) return;
    if (blob->native_blob) {
        SkRect r = blob->native_blob->bounds();
        out[0] = r.x();       /* left */
        out[1] = r.y();       /* top */
        out[2] = r.width();   /* width */
        out[3] = r.height();  /* height */
    } else {
        out[0] = out[1] = out[2] = out[3] = 0;
    }
}

extern "C" int cevg_text_blob_get_run_count(const CevgTextBlob* blob) {
    if (!blob) return 0;
    return (blob->run_count > 0) ? blob->run_count : 1;
}

extern "C" void cevg_text_blob_get_runs(const CevgTextBlob* blob, CevgTextRun* out,
                                        int count, size_t run_stride) {
    if (!blob || !out || count <= 0 || run_stride == 0) return;

    /* Walk the caller's array by the stride IT compiled with, writing each
     * record field-by-field. This is what keeps array traversal correct in
     * an already-compiled binary if CevgTextRun gains fields later: we never
     * assume the caller's element size equals our sizeof(CevgTextRun).
     * We copy min(our fields, caller's stride) by capping at run_stride. */
    auto* base = reinterpret_cast<unsigned char*>(out);
    const size_t writable = run_stride;  /* bytes available per element */

    auto write_run = [&](int i, const CevgTextRun& src) {
        if (i >= count) return;
        unsigned char* slot = base + (size_t)i * run_stride;
        /* Only assign through fields that fit in the caller's stride. Since
         * all current fields precede any future ones, a simple size gate per
         * field is sufficient and forward-safe. */
        CevgTextRun tmp = src;
        size_t copy = sizeof(CevgTextRun) < writable ? sizeof(CevgTextRun) : writable;
        memcpy(slot, &tmp, copy);
    };

    int n = (blob->run_count > 0 && blob->runs) ? blob->run_count : 1;
    if (n > count) n = count;

    if (blob->runs && blob->run_count > 0) {
        for (int i = 0; i < n; i++) write_run(i, blob->runs[i]);
    } else {
        CevgTextRun single;
        single.glyph_start = 0;
        single.glyph_count = blob->glyph_count;
        single.byte_start  = 0;
        single.byte_end    = 0;
        single.dir         = kCevgDir_LTR;
        write_run(0, single);
    }
}

/* Line break using SkUnicode (UAX #14 compliant via Skia's internal ICU).
 * Uses SkUnicodes::ICU::Make() to create the SkUnicode instance, then
 * computeCodeUnitFlags() to get per-code-unit flags including
 * kSoftLineBreakBefore / kHardLineBreakBefore, and converts those
 * back to UTF-8 byte offsets. Falls back to a simple whitespace/CJK
 * heuristic if SkUnicode is unavailable. */
extern "C" int cevg_text_find_line_breaks(const char* text, size_t len,
                                int* out_breaks, int max_breaks) {
    if (!text || len == 0) return 0;

    /* Try SkUnicode for proper UAX #14 line breaking. */
    auto uni = SkUnicodes::ICU::Make();
    if (uni) {
        /* computeCodeUnitFlags works on a mutable copy of the text. */
        std::vector<char> mutableText(text, text + len);
        mutableText.push_back('\0');
        skia_private::TArray<SkUnicode::CodeUnitFlags, true> flags;
        if (uni->computeCodeUnitFlags(mutableText.data(), (int)len, /*replaceTabs=*/true, &flags)) {
            int count = 0;
            for (int i = 0; i < flags.size() && i < (int)len; i++) {
                auto f = flags[i];
                if (SkUnicode::hasSoftLineBreakFlag(f) || SkUnicode::hasHardLineBreakFlag(f)) {
                    /* i is the UTF-8 byte offset of the code unit that has a
                     * line break before it. The break position is after the
                     * previous codepoint, which is at byte offset i. */
                    if (i > 0 && i <= (int)len) {
                        if (out_breaks && count < max_breaks)
                            out_breaks[count] = i;
                        count++;
                    }
                }
            }
            return count;
        }
    }

    /* Fallback: simple whitespace/CJK heuristic. */
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        bool is_break = (c == ' ' || c == '\t');
        if (!is_break && c >= 0xE4 && c <= 0xE9 && i + 2 < len)
            is_break = true;
        if (is_break && i + 1 < len) {
            if (out_breaks && count < max_breaks)
                out_breaks[count] = (int)(i + 1);
            count++;
        }
    }
    return count;
}

/* ===================================================================
 * Image
 * =================================================================== */
extern "C" CevgResult cevg_image_create_from_file(const char* path, CevgImage** out_image) {
    if (!path || !out_image) return kCevgErrorInvalidArg;
    *out_image = nullptr;

    auto skData = SkData::MakeFromFileName(path);
    if (!skData) return kCevgErrorFileNotFound;
    auto img = SkImages::DeferredFromEncodedData(skData);
    if (!img) return kCevgErrorDecodeFailed;

    CevgImage* image = new (std::nothrow) CevgImage();
    if (!image) return kCevgErrorOOM;
    image->image = std::move(img);
    image->width = image->image->width();
    image->height = image->image->height();
    *out_image = image;
    return kCevgSuccess;
}

extern "C" CevgResult cevg_image_create_from_memory(const void* data, size_t len,
                                         CevgImage** out_image) {
    if (!data || !len || !out_image) return kCevgErrorInvalidArg;
    *out_image = nullptr;

    auto skData = SkData::MakeWithCopy(data, len);
    auto img = SkImages::DeferredFromEncodedData(skData);
    if (!img) return kCevgErrorDecodeFailed;

    CevgImage* image = new (std::nothrow) CevgImage();
    if (!image) return kCevgErrorOOM;
    image->image = std::move(img);
    image->width = image->image->width();
    image->height = image->image->height();
    *out_image = image;
    return kCevgSuccess;
}

extern "C" CevgImage* cevg_image_create_from_pixels(const void* pixels, int w, int h) {
    if (!pixels || w <= 0 || h <= 0) return nullptr;

    SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType,
                                         kUnpremul_SkAlphaType,
                                         SkColorSpace::MakeSRGB());
    size_t rowBytes = info.minRowBytes();
    auto skData = SkData::MakeWithCopy(pixels, (size_t)w * h * 4);
    auto img = SkImages::RasterFromData(info, skData, rowBytes);
    if (!img) return nullptr;

    CevgImage* image = new (std::nothrow) CevgImage();
    if (!image) return nullptr;
    image->image = std::move(img);
    image->width = w;
    image->height = h;
    return image;
}

extern "C" void cevg_image_ref(CevgImage* image) {
    if (!image) return;
    image->ref_count.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void cevg_image_unref(CevgImage* image) {
    if (!image) return;
    /* Release on the decrement, acquire fence before destruction, so the
     * thread that frees sees all prior writes from other threads (mirrors
     * the typeface refcount). */
    if (image->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete image;
    }
}

extern "C" void cevg_image_destroy(CevgImage* image) {
    cevg_image_unref(image);   /* drop the caller's (creation) reference */
}

extern "C" int cevg_image_get_width(const CevgImage* image) {
    return image ? image->width : 0;
}

extern "C" int cevg_image_get_height(const CevgImage* image) {
    return image ? image->height : 0;
}

/* ===================================================================
 * GPU Profiler
 * =================================================================== */
#ifdef CEVG_VULKAN
extern "C" void cevg_gpu_profiler_enable(CevgContext* ctx, bool enable) {
    if (!ctx || !ctx->device) return;
    /* The query-pool / timestamp commands are not part of the required loader
     * set (they need VK_EXT_host_query_reset / Vulkan 1.2). If the host's device
     * didn't expose them, profiling is unavailable rather than a null call. */
    if (enable && (!ctx->vk.CreateQueryPool || !ctx->vk.ResetQueryPool ||
                   !ctx->vk.CmdWriteTimestamp || !ctx->vk.GetQueryPoolResults ||
                   !ctx->vk.DestroyQueryPool)) {
        ctx->profilerEnabled = false;
        return;
    }
    ctx->profilerEnabled = enable;
    if (enable && ctx->profilerQueryPool == VK_NULL_HANDLE) {
        /* Create the query pool lazily: 2 timestamps per ring slot. */
        VkQueryPoolCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qi.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qi.queryCount = CevgContext::kProfilerRingSize * 2;
        ctx->vk.CreateQueryPool(ctx->device, &qi, nullptr, &ctx->profilerQueryPool);
        /* Reset all queries so the first read doesn't return garbage.
         * vkResetQueryPool is core since Vulkan 1.2. */
        ctx->vk.ResetQueryPool(ctx->device, ctx->profilerQueryPool, 0,
                         CevgContext::kProfilerRingSize * 2);
    }
}

extern "C" void cevg_gpu_profiler_begin_frame(CevgContext* ctx) {
    if (!ctx || !ctx->profilerEnabled || ctx->profilerQueryPool == VK_NULL_HANDLE) return;

    /* Read back results from 2 frames ago (the queries should be available
     * by now since we double-buffer the swapchain). The ring index has
     * already been advanced by cevg_swapchain_present at the end of the
     * previous frame, so "2 frames ago" is (current - 2). */
    int readSlot = (ctx->profilerRingIndex - 2 + CevgContext::kProfilerRingSize)
                   % CevgContext::kProfilerRingSize;
    uint64_t results[2] = {0, 0};
    VkResult r = ctx->vk.GetQueryPoolResults(
        ctx->device, ctx->profilerQueryPool,
        readSlot * 2, 2,
        sizeof(results), results, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (r == VK_SUCCESS) {
        uint64_t beginTs = results[0];
        uint64_t endTs   = results[1];
        if (endTs > beginTs) {
            uint64_t diffTicks = endTs - beginTs;
            double ns = (double)diffTicks * (double)ctx->timestampPeriod;
            ctx->profilerLastMs = (float)(ns / 1e6);
            ctx->profilerHasData = true;
        }
    }
}
#else
extern "C" void cevg_gpu_profiler_enable(CevgContext* ctx, bool enable) {
    (void)ctx; (void)enable;
}

extern "C" void cevg_gpu_profiler_begin_frame(CevgContext* ctx) {
    (void)ctx;
}
#endif

extern "C" void cevg_gpu_profiler_end_frame(CevgContext* ctx) {
    /* No-op: the ring index is advanced by cevg_swapchain_present, which
     * is the single source of truth for when a frame's GPU work is
     * complete. The begin/end timestamps are written in the swapchain's
     * command buffer (vkCmdWriteTimestamp), so they bracket the actual
     * blit+present GPU work. */
}

extern "C" float cevg_gpu_profiler_get_frame_time(const CevgContext* ctx) {
    if (!ctx || !ctx->profilerHasData) return 0.0f;
    return ctx->profilerLastMs;
}