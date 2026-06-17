# Cevg — Skia Graphite vector-graphics library

A C ABI vector-graphics library wrapping Skia Graphite. Ships as a single
shared library (`libcevg.so` / `cevg.dll`) with zero external C++ dependencies
— all Skia, ICU, HarfBuzz, FreeType, and other third-party code is statically
linked and symbol-hidden. Only `cevg_*` symbols are exported.

## Backends

| Backend | Status | Config |
|---------|--------|--------|
| **CPU raster** | Working | Default (no Vulkan needed) |
| **Vulkan GPU** | Working | `-DCEVG_VULKAN=ON` |

CPU-only mode is the default. Vulkan is opt-in for GPU-accelerated rendering
with swapchain present.

## Quick start

```bash
# CPU-only build (default)
cmake -B build -G Ninja -DCEVG_SHARED=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build

# With Vulkan GPU support
cmake -B build -G Ninja -DCEVG_SHARED=ON -DCEVG_VULKAN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build

# With internal tests
cmake -B build -G Ninja -DCEVG_SHARED=ON -DCEVG_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `CEVG_SHARED` | `OFF` | Build as shared library |
| `CEVG_VULKAN` | `ON` | Enable Vulkan GPU backend |
| `CEVG_BUILD_TESTS` | `OFF` | Build internal test suite |

## ABI isolation

The shared library is fully ABI-isolated:

- **Static C++ runtime**: `-static-libstdc++ -static-libgcc` (GCC/Clang)
- **Symbol visibility**: `-fvisibility=hidden` + version script (`cevg.version`
  on Linux, `cevg.exported_symbols` on macOS, `cevg.def` on Windows)
- **Only `cevg_*` exported**: all internal Skia/ICU/HarfBuzz/FreeType symbols
  are hidden under `local: *`
- **PIC globally**: `CMAKE_POSITION_INDEPENDENT_CODE ON` when `CEVG_SHARED=ON`

Runtime dependencies (Linux): `libc`, `libm`, `libfontconfig.so.1` (+ transitive
`libfreetype`, `libexpat`, `libuuid`). No `libstdc++`, no `libskia`.

## Text rendering

Text is shaped via HarfBuzz with ICU Unicode support:

- **Font discovery**: DirectWrite (Windows), CoreText (macOS/iOS),
  FontConfig+FreeType (Linux) — selected at build time
- **Shaper**: `SkShaper::MakeShapeThenWrap` (HarfBuzz) with BiDi, script, and
  font-fallback iterators; falls back to `MakePrimitive` if HarfBuzz is
  unavailable
- **ICU data**: embedded in the binary on Linux/macOS (`.incbin`), runtime file
  on Windows
- **TTC support**: `cevg_typeface_create_from_file(path, ttc_index)` — pass
  `ttc_index=0` for regular fonts, higher indices for TTC/OTC collections
- **Baseline convention**: `draw_text_blob(x, y)` treats y as the baseline
  (Skia standard), not the top of the text
- **Width measurement**: `cevg_text_blob_get_width` returns the pen-advance
  width (where the pen rests after the last glyph), computed as
  `max(glyph_origin + advance)`. This is the correct typographic width for
  line-breaking and layout — not `SkTextBlob::bounds()`, which returns a
  conservative ink bounding box that can be 30%+ wider
- **Per-glyph advances**: `cevg_text_blob_get_glyph_advances` returns real
  advance widths from `SkFont::getWidths` per run, not reconstructed from
  position differences. Last glyph advance is correct (not inflated)
- **Ink bounds**: `cevg_text_blob_get_ink_bounds` exposes the conservative ink
  bounding box separately — useful for collision detection, not for layout
- **Hit testing**: `cevg_text_blob_hit_test` uses advance-based half-cell logic
  for cursor positioning. Returns UTF-8 byte offset; past the last glyph
  returns `text_len` for end-of-line cursor

```c
CevgTypeface* face = cevg_typeface_create_from_file("/path/to/font.ttf", 0);
CevgTextBlob* blob = cevg_text_blob_make("Hello!", 6, face, 24.0f, kCevgDir_LTR);
cevg_canvas_draw_text_blob(canvas, blob, x, y, paint);
```

## Binary (ABI) stability

The library ships to callers that cannot be recompiled, so the public ABI is
designed to be extended for years without breaking compiled binaries:

1. **Versioned structs.** Every caller-allocated struct (`CevgConfig`,
   `CevgVulkanDevice`, `CevgFontMetrics`) starts with `uint32_t struct_size`.
   The library reads only the fields that size covers — new fields can be
   appended forever.

2. **Backend hidden behind opaque handles.** Vulkan is never named in any type
   you embed by value. Wrap your device with `cevg_gpu_device_create_vulkan()`
   and hand the opaque `CevgGpuDevice*` to `CevgConfig`.

3. **Stride-based arrays.** Calls like `cevg_text_blob_get_runs(blob, out,
   count, sizeof(CevgTextRun))` take the element stride from the caller.

The ABI stability contract is documented at the top of `cevg.h`.

## Typical frame (CPU)

```c
CevgConfig cfg = {0};
cfg.struct_size = sizeof(cfg);
cfg.color_space = kCevgColorSpace_sRGB;
CevgContext* ctx = cevg_context_create(&cfg);

CevgSurface* surf = cevg_surface_create(ctx, 800, 600);
CevgCanvas* c = cevg_canvas_create(surf);
cevg_canvas_clear(c, 0, 0, 0, 1);

/* Draw shapes, text, images... */
CevgPaint* p = cevg_paint_create();
cevg_paint_set_color(p, 1, 0, 0, 1);
cevg_canvas_draw_rect(c, &(CevgRect){10,10,200,100}, p);
cevg_paint_destroy(p);

cevg_canvas_flush(c);
cevg_canvas_destroy(c);

/* Read pixels if needed */
uint8_t* pixels = cevg_surface_read_pixels(surf, NULL);

cevg_surface_destroy(surf);
cevg_context_destroy(ctx);
```

## Typical frame (Vulkan windowed)

```c
CevgVulkanDevice vk = {0};
vk.struct_size        = sizeof(vk);
vk.vk_instance        = myInstance;
vk.vk_physical_device = myPhysDevice;
vk.vk_device          = myDevice;
vk.vk_queue           = myGraphicsQueue;
vk.vk_queue_index     = myGraphicsQueueFamily;
CevgGpuDevice* gpu = cevg_gpu_device_create_vulkan(&vk);

CevgConfig cfg = {0};
cfg.struct_size = sizeof(cfg);
cfg.device      = gpu;
cfg.color_space = kCevgColorSpace_sRGB;
CevgContext* ctx = cevg_context_create(&cfg);

CevgSurface* surf = cevg_surface_create_for_vk_surface(
    ctx, vkSurface, width, height, kCevgPresentMode_Fifo);

while (running) {
    CevgCanvas* c = cevg_canvas_create(surf);
    if (!c) { cevg_surface_resize(surf, w, h); continue; }
    cevg_canvas_clear(c, 0, 0, 0, 1);
    /* ... draw ... */
    cevg_canvas_destroy(c);

    if (cevg_surface_present(surf) == kCevgErrorDeviceLost)
        cevg_surface_resize(surf, w, h);
    cevg_context_tick(ctx);
}
```

## Key features

- **Multi-recorder support**: one recorder per worker thread for parallel draw
  recording; only submit/present touch the GPU queue
- **Resolved-paint cache**: paints that don't change between draws are converted
  to `SkPaint` exactly once, keyed on `CevgPaint::generation`
- **Retained display lists** (`CevgDisplayList`, SkPicture-backed) with
  per-replay matrix for cheap scrolling/transform animation
- **`cevg_canvas_draw_image_nine`**: manual 9-rect decomposition (Skia's
  `drawImageNine` is a no-op on Graphite)
- **Font fallback**: `cevg_text_blob_make_ex` accepts fallback typefaces for
  multi-script rendering

## Third-party dependencies (all statically linked)

| Library | Version | Purpose |
|---------|---------|---------|
| Skia | m150 | Rendering engine (Graphite backend) |
| ICU | 74.2 | Unicode support (BiDi, break iterators, locale data) |
| HarfBuzz | bundled with Skia | Text shaping |
| FreeType | bundled with Skia | Font rasterization |
| FontConfig | system | Linux system font discovery |
| expat | bundled | XML parsing (SVG, font config) |
| libjpeg-turbo | bundled | JPEG decode |
| libpng | bundled | PNG decode |
| libwebp | bundled | WebP decode |
| zlib | bundled | Compression |

## Project structure

```
cevg/
├── include/cevg/cevg.h          # Public C ABI header
├── src/cevg_skia_graphite.cpp   # Backend implementation (~3300 lines)
├── tests/
│   ├── test_cevg_render.c       # 144-test rendering suite
│   ├── test_cevg_overlap.c      # 144-test compositing suite
│   └── test_text_measure.c      # Text measurement verification (visual)
├── third/skia/
│   ├── CMakeLists.txt           # Skia + deps build
│   ├── skia-m150/               # Skia source
│   └── third/icu/               # ICU source + data
├── cevg.version                 # Linux version script
├── cevg.exported_symbols        # macOS export list
├── cevg.def                     # Windows export list
├── CMakeLists.txt               # Main build
└── docs/
    └── skia-m150-porting-pitfalls.md  # Porting notes for m151+
```

## Planned: Lottie & SVG

Lottie (Skottie) and SVG rendering are intended future features. They will
arrive purely additively — new opaque handles and functions, no existing ABI
changes. `CevgResourceProvider` (in `cevg.h`) is already defined as the
ABI-safe C equivalent of Skia's `skresources::ResourceProvider`.
