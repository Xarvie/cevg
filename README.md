# Cevg — Graphite/Vulkan rewrite

A single-backend rewrite of the Cevg vector-graphics library. The legacy
GLES3 backend is gone; the library now targets **Skia Graphite on Vulkan**
and nothing else. The C ABI is preserved, so existing callers keep
linking — only the engine binding underneath changed, plus a handful of
new entry points.

## What changed vs. the old library

### Removed
- **The entire GLES3 backend** (`es/`) and the CMake backend-selection
  switch. One backend, one code path.
- **The CPU-readback present path.** The old `cevg_surface_present()` did
  `submit(SyncToCpu::kYes)` → `asyncRescaleAndReadPixels` (a full-frame
  GPU→CPU readback) → `memcpy` → `SDL_Texture` upload → SDL present. Every
  frame round-tripped the whole framebuffer through system memory and
  stalled the CPU on the GPU. That is all gone.
- `cevg_surface_create_from_native`, `cevg_surface_create_for_window`,
  `cevg_surface_set_present_data`, and the `CevgPresentDataSkia` struct —
  these only existed to feed the old present path.
- The hardcoded Windows-only `SkFontMgr_New_DirectWrite()`.

### New / fixed
- **GPU-direct present** via a real Vulkan swapchain. A window surface
  owns a `VkSwapchainKHR`; `cevg_surface_present()` snaps the Graphite
  recording, submits it with `SyncToCpu::kNo`, then blits the rendered
  image into the acquired swapchain image and `vkQueuePresentKHR`s — all
  ordered by GPU semaphores. The CPU never waits; pixels never leave VRAM.
- **Portable font manager**: DirectWrite on Windows, CoreText on Apple,
  Fontconfig+FreeType elsewhere — selected at build time.
- **Multi-recorder support**: the context owns a default recorder, but you
  can create one recorder per worker thread and bind a canvas to it, so
  draw recording parallelizes. Only submit/present touch the GPU queue.
- **Resolved-paint cache** keyed on `CevgPaint::generation` — a paint that
  doesn't change between draws is converted to `SkPaint` (with its
  shader/colorfilter/imagefilter) exactly once.
- **Retained display lists** (`CevgDisplayList`, SkPicture-backed) with a
  per-replay matrix, so scrolling and transform animation re-issue a cheap
  recorded list instead of re-running your draw calls.
- **`cevg_canvas_draw_image_nine`** is now implemented as a manual 9-rect
  decomposition, because `SkCanvas::drawImageNine` is a no-op on Graphite.

## Binary (ABI) stability — built for long-term freeze

This library ships as a `.dll`/`.so` to callers that cannot be recompiled,
so the public ABI is designed to be extended for years without ever
breaking an already-compiled binary. Three mechanisms make that possible:

1. **Versioned structs.** Every caller-allocated struct (`CevgConfig`,
   `CevgVulkanDevice`, `CevgFontMetrics`) starts with `uint32_t struct_size`,
   which the caller sets to `sizeof(...)`. The library reads only the fields
   that size covers, so new fields can be appended forever — old callers
   keep working, and a new caller against an old library degrades cleanly.
   **You must set `struct_size` on every such struct** (the examples do).

2. **Backend hidden behind an opaque handle.** Vulkan is never named in any
   type you embed by value. You wrap your device once with
   `cevg_gpu_device_create_vulkan()` and hand the resulting opaque
   `CevgGpuDevice*` to `CevgConfig`. If a Metal/Dawn/WebGPU backend is ever
   added, it appears as a new `cevg_gpu_device_create_*()` factory and
   nothing else in the header changes — your Vulkan code still links.

3. **Stride-based arrays.** Array-returning calls like
   `cevg_text_blob_get_runs(blob, out, count, sizeof(CevgTextRun))` take the
   element stride from the caller, so the element struct can gain fields
   without corrupting array indexing in old binaries.

The rules for evolving the header safely (append-only, padding caveats,
enum/function growth) are written at the top of `cevg.h` under
**ABI STABILITY CONTRACT**. `tests/abi_test.c` is a pure-C regression test
that proves the size-gate works and that frozen field offsets never move —
run it after any change to a public struct:

```
cc -std=c11 -Wall -Wextra -Iinclude tests/abi_test.c -o abi_test && ./abi_test
```

## API additions (all in `cevg.h`)

```c
/* Wrap a host-created Vulkan device, then hand the OPAQUE result to config.
 * The Vulkan struct is consumed once and never embedded long-term. */
typedef struct CevgGpuDevice_ CevgGpuDevice;
CevgGpuDevice* cevg_gpu_device_create_vulkan(const CevgVulkanDevice*);
void           cevg_gpu_device_destroy(CevgGpuDevice*);

CevgRecorder* cevg_recorder_create(CevgContext*);
void          cevg_recorder_destroy(CevgRecorder*);
CevgRecorder* cevg_context_default_recorder(CevgContext*);
CevgCanvas*   cevg_canvas_create_with_recorder(CevgSurface*, CevgRecorder*);

/* Real swapchain present; vk_surface is a host-created VkSurfaceKHR. */
CevgSurface*  cevg_surface_create_for_vk_surface(CevgContext*, void* vk_surface,
                                                 int w, int h, CevgPresentMode);
CevgResult    cevg_surface_resize(CevgSurface*, int w, int h);
CevgResult    cevg_surface_present(CevgSurface*);          /* now returns CevgResult */
void          cevg_surface_add_dirty_rect(CevgSurface*, int x,int y,int w,int h);

CevgResult    cevg_context_reset(CevgContext*);            /* device-loss recovery */
void          cevg_context_tick(CevgContext*);             /* per-frame cleanup */
size_t        cevg_context_get_vram_used(const CevgContext*);

CevgCanvas*      cevg_display_list_record_begin(CevgContext*, int w, int h);
CevgDisplayList* cevg_display_list_record_end(CevgCanvas* capture);
void             cevg_display_list_destroy(CevgDisplayList*);
void             cevg_canvas_draw_display_list(CevgCanvas*, const CevgDisplayList*,
                                               const float matrix[9]);
```

## Typical frame (windowed)

```c
/* 1. Wrap your host-created Vulkan device (consumed once, then discardable). */
CevgVulkanDevice vk = {0};
vk.struct_size        = sizeof(vk);          /* REQUIRED for ABI versioning */
vk.vk_instance        = myInstance;
vk.vk_physical_device = myPhysDevice;
vk.vk_device          = myDevice;
vk.vk_queue           = myGraphicsQueue;
vk.vk_queue_index     = myGraphicsQueueFamily;
CevgGpuDevice* gpu = cevg_gpu_device_create_vulkan(&vk);

/* 2. Create the context against the OPAQUE device (no Vulkan type here). */
CevgConfig cfg = {0};
cfg.struct_size = sizeof(cfg);               /* REQUIRED for ABI versioning */
cfg.device      = gpu;
cfg.color_space = kCevgColorSpace_sRGB;
CevgContext* ctx = cevg_context_create(&cfg);

/* vkSurface is a VkSurfaceKHR you created from your window. */
CevgSurface* surf = cevg_surface_create_for_vk_surface(
    ctx, vkSurface, width, height, kCevgPresentMode_Fifo);

while (running) {
    CevgCanvas* c = cevg_canvas_create(surf);   /* acquires next swap image */
    if (!c) { cevg_surface_resize(surf, w, h); continue; }  /* out-of-date */
    cevg_canvas_clear(c, 0,0,0,1);
    /* ... draw ... */
    cevg_canvas_destroy(c);

    if (cevg_surface_present(surf) == kCevgErrorDeviceLost)
        cevg_surface_resize(surf, w, h);
    cevg_context_tick(ctx);                     /* steady-state memory */
}
```

## Planned: Lottie & SVG (and the resource-callback hook)

Lottie (Skottie) and SVG rendering are intended future features. They will
arrive **purely additively** — new opaque handles (`CevgAnimation`,
`CevgSvg`) and new `cevg_animation_*` / `cevg_svg_*` functions on top of the
existing `CevgContext`/`CevgCanvas`. None of the current ABI changes when
they land, so nothing needs to be done for them now.

The one piece pulled forward is `CevgResourceProvider` (in `cevg.h`),
defined now because **callback signatures are the one thing that genuinely
hurts to add after hosts implement them.** Lottie/SVG files reference
external images, fonts, and nested-animation blobs by name; this struct is
the ABI-safe C equivalent of Skia's `skresources::ResourceProvider`. The
design keeps all Skia types out of the boundary: the host is asked only to
return encoded **bytes** for a named resource, and Cevg turns those into the
decoded image / typeface / parsed animation itself. Cevg copies the returned
bytes before the callback returns, so the host can free its buffer
immediately. It is a versioned struct (`struct_size` + append-only
callbacks), so a host that implements only `load_image` keeps working when
more callbacks are added later. When the Lottie/SVG calls are introduced, a
`CevgResourceProvider*` will be an optional argument to their creation
functions (NULL = embedded/data-URI resources only).

`CMakeLists.txt` carries a commented-out block listing the extra Skia
libraries those features will link (`skottie`, `sksg`, `skshaper`,
`skresources`, the `svg` module) so wiring them up later is mechanical.



```
cmake -B build -DSKIA_DIR=/path/to/your/skia/output
cmake --build build
```

Point `SKIA_DIR` at the same prebuilt Skia tree the old backend used
(`lib/`, `include/`, `gen/`). On Windows it links Skia's bundled
`vulkan-1.lib`; on Linux/macOS it links the system Vulkan loader.

## Build-time verification points

The bulk of the backend uses the **stable public** Skia API. The only
checkout-sensitive area is the Vulkan↔Graphite swapchain glue, because
Graphite's Vulkan headers still move between Skia revisions. If the first
compile/link fails, check these against *your* Skia headers — the fix is
always a signature/field-name tweak, not a design change:

1. **`graphite::BackendTextures::MakeVulkan(...)`** — argument list and the
   `skgpu::VulkanAlloc{}` last parameter. Some revisions take a
   `VulkanTextureInfo` by const-ref vs. value, or omit the alloc.
   (`cevg_swapchain_build`.)
2. **`SkSurfaces::WrapBackendTexture(recorder, tex, colorType, colorSpace,
   props)`** — recent Skia dropped the explicit `SkColorType`; if yours has,
   remove that argument. (`cevg_swapchain_build`.)
3. **`graphite::VulkanTextureInfo` field names** — `fImageUsageFlags`,
   `fFormat`, `fImageTiling`, `fSharingMode`, `fFlags`. Adjust if your
   header spells them differently. (`cevg_swapchain_build`.)
4. **`Recorder::currentBudgetedBytes()` / `performDeferredCleanup(...)`**
   and **`Context::checkAsyncWorkCompletion()`** — names of the
   memory/cleanup calls. If absent, stub `cevg_context_get_vram_used` to 0
   and make `cevg_context_tick` call only what exists.
   (`cevg_context_get_vram_used`, `cevg_context_tick`.)
5. **`SkPictureRecorder::finishRecordingAsPicture()`** — stable, but confirm
   the include path `include/core/SkPictureRecorder.h`.
   (`cevg_display_list_record_end`.)

Everything else (paint/path/shader/text/image, the swapchain Vulkan calls
themselves) is standard public Skia or standard Vulkan and should compile
as-is against the Skia build the old backend already linked against.

## Files

| File | Role |
|---|---|
| `include/cevg/cevg.h` | Public C ABI (frozen names + new entry points) |
| `common/cevg_internal.h` | Shared Path/Paint struct bodies |
| `common/cevg_common.c` | Backend-agnostic Path/Paint (reused verbatim) |
| `src/cevg_skia_graphite.cpp` | The Graphite/Vulkan backend (~2000 lines) |
| `cevg.def` | MSVC export list (117 symbols) |
| `CMakeLists.txt` | Single-backend build |
