# Skia m150 CMake 移植踩坑记录

> 供未来 m151+ 移植参考。按严重程度排序。

---

## 1. ICU 数据嵌入必须对齐（致命 — 文字 0 glyphs）

**现象**：HarfBuzz shaper 返回非空，所有迭代器非空，`shape()` 被调用，但 `commitRunBuffer` 从未触发，产出 0 glyphs。

**根因**：`.incbin` 嵌入 `icudtl.dat` 时没有加对齐指令。ICU 的 `ucptrie_openFromBinary` 内部检查 `U_POINTER_MASK_LSB(data, 3) != 0`，要求 trie 数据地址 4 字节对齐。未对齐时直接返回 `U_ILLEGAL_ARGUMENT_ERROR`，导致 `ubrk_open` 失败 → `makeBreakIterator` 返回 null → `ShapeThenWrap::wrap` 提前退出。

**修复**：`.S` 文件中 `.incbin` 前加 `.balign 16`：
```asm
.section .rodata
.balign 16
icudt74_dat:
    .incbin "icudtl.dat"
```

**检查方法**：新版本先确认 `ubrk_open(UBRK_LINE, "en", nullptr, 0, &status)` 返回 `U_ZERO_ERROR`。

---

## 2. RunHandler buffer 必须与 blob handler 共享（致命 — 文字乱码）

**现象**：文字像素存在但字形错乱，字母重叠叠加，不可辨认。

**根因**：`CevgRunHandler::runBuffer()` 自己分配了一套 glyphs/positions buffer 返回给 shaper 写入，同时 `fBlobHandler.runBuffer()` 分配了另一套从未被 shaper 写入的 buffer。`native_blob`（用于 `drawTextBlob` 实际绘制）从第二套未初始化 buffer 构建，数据全是垃圾。

**修复**：`runBuffer()` 直接返回 `fBlobHandler.runBuffer()` 的 buffer：
```cpp
Buffer runBuffer(const RunInfo& info) override {
    Buffer buf = fBlobHandler.runBuffer(info);
    fCurrentBuffer = buf;  // 保存指针供 commitRunBuffer 读 metadata
    return buf;
}
```

**原则**：自定义 RunHandler 如果同时转发给内部 handler，必须共享同一套 buffer，不能各分配各的。

---

## 3. U_ENABLE_DYLOAD=0 必须定义（致命 — ICU 数据找不到）

**现象**：ICU 数据已嵌入二进制，但运行时 `dlopen/dlsym` 查找 `icudt74_dat` 符号失败（version script 把它隐藏了）。

**根因**：ICU 默认 `U_ENABLE_DYLOAD=1`，运行时用 `dlopen` 动态查找数据。嵌入数据后不需要动态加载，且 version script 的 `local: *` 隐藏了 `icudt74_dat`，`dlsym` 找不到。

**修复**：添加 `U_ENABLE_DYLOAD=0` 编译定义。Skia GN 在 `BUILD.gn:79` 明确定义了此值。

**检查方法**：新版本移植时，对照 Skia GN 的 `defines` 列表，确保所有 ICU 相关宏一致。

---

## 4. ICU_VERSION 必须与源码版本一致（潜在问题）

**现象**：`ICU_VERSION` 设为 `78.1` 但实际源码是 `74.2`。

**影响**：当前 CMake 构建未使用 `find_package(ICU)`，所以没直接报错，但版本不一致可能导致未来问题。

**修复**：`set(ICU_VERSION "74.2")` 与 `third/icu/source/common/unicode/uvernum.h` 中的 `U_ICU_VERSION` 保持一致。

**检查方法**：新版本移植时，检查 `uvernum.h` 中的版本号。

---

## 5. FontConfig 源文件必须在 SKIA_PORT_SOURCES set() 之后追加

**现象**：`SkFontMgr_New_FontConfig` 链接时 undefined symbol。

**根因**：`list(APPEND SKIA_PORT_SOURCES ...)` 写在了 `set(SKIA_PORT_SOURCES ...)` 之前，APPEND 的内容被后续 set 覆盖。

**修复**：把 FontConfig 源文件追加移到 `set(SKIA_PORT_SOURCES ...)` 之后。

---

## 6. SkFontMgr_New_FontConfig API 变更

**现象**：m150 中 `SkFontMgr_New_FontConfig(nullptr)` 编译失败，需要两个参数。

**根因**：m150 的 API 是 `SkFontMgr_New_FontConfig(FcConfig*, std::unique_ptr<SkFontScanner>)`，需要显式传入 `SkFontScanner_Make_FreeType()`。

**修复**：
```cpp
SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
```

**检查方法**：新版本移植时，检查 `SkFontMgr_fontconfig.h` 中的函数签名是否变化。

---

## 7. SkTypeface_proxy.cpp 必须与 SkFontMgr_fontconfig.cpp 一起编译

**现象**：`SkTypeface_proxy` 相关符号 undefined。

**根因**：`SkFontMgr_fontconfig.cpp` 依赖 `SkTypeface_proxy.cpp`，只加前者不加后者会链接失败。

**修复**：两个文件一起加入 `SKIA_PORT_SOURCES`。

---

## 8. expat entropy 函数与 clang 冲突

**现象**：`arc4random_buf` 等函数声明冲突。

**根因**：expat 默认检测系统是否有 `arc4random_buf`/`getrandom`/`syscall(__NR_getrandom)`，clang 编译时声明冲突。

**修复**：覆盖 expat 的检测，与 Skia GN 保持一致：
```cmake
target_compile_definitions(expat PRIVATE
    HAVE_ARC4RANDOM_BUF=0
    HAVE_GETRANDOM=0
    HAVE_SYSCALL_GETRANDOM=0
    XML_DEV_URANDOM=1
)
```

---

## 9. Ninja 下 version script 路径需要 SHELL: 前缀

**现象**：Ninja 构建时 version script 路径含空格或特殊字符导致链接失败。

**修复**：
```cmake
target_link_options(cevg PRIVATE "SHELL:-Wl,--version-script=${CMAKE_SOURCE_DIR}/cevg.version")
```

---

## 10. Linux 测试链接需要 -lm

**现象**：`test_cevg_render` 链接失败，`fabsf` undefined。

**修复**：`target_link_libraries(test_cevg_render m)`

---

## 11. SkTextBlob::bounds() 不是笔进宽度（严重 — 文字测量偏大 30%+）

**现象**：`cevg_text_blob_get_width` 返回的宽度比实际笔进宽大约 30%。例如 "MMM" 64px 返回 224.78，而真实笔进宽仅 165.00。末位字形 advance 异常膨胀（同一个 M，末位=35.9、内部=17）。

**根因**：`SkTextBlob::bounds()` 返回的是**保守墨迹包围盒**——它取字体中所有字形的全局极值（最大左侧轴承、最大右侧延伸），保证"任何字形的墨迹都不会超出此框"。这不是排版宽度（pen-advance width）。用它当宽度会导致：
- 宽度偏大 30%+
- 末位字形 advance 被迫吸收全部过量（`width - positions_x[last]`）
- 换行/省略/光标定位全部不准

**修复**：用 `max(glyph_origin + its_advance)` 计算笔进宽。advance 从 `SkFont::getWidths` 逐 run 取真实值，而非从位置差重建：
```cpp
// 在 CevgRunHandler::commitRunBuffer 中收集真实 advance
if (info.glyphCount > 0) {
    std::vector<SkScalar> adv(info.glyphCount);
    info.fFont.getWidths(
        SkSpan<const SkGlyphID>{fCurrentBuffer.glyphs, (size_t)info.glyphCount},
        SkSpan<SkScalar>{adv.data(), (size_t)info.glyphCount});
    for (int i = 0; i < info.glyphCount; ++i)
        fResult->advances.push_back(adv[i]);
}

// 在 cevg_shape_text 中计算笔进宽
float w = 0.0f;
for (size_t i = 0; i < result->positions.size(); ++i) {
    float adv = (i < result->advances.size()) ? result->advances[i] : 0.0f;
    w = std::max(w, result->positions[i].x() + adv);
}
result->shapedWidth = w;
```

**注意**：`bounds()` 仍然有用——它是保守墨迹盒，适合碰撞检测。通过 `cevg_text_blob_get_ink_bounds` 单独暴露。

---

## 12. SkFont::getWidths API 在 m150 中改为 SkSpan（编译失败）

**现象**：调用 `info.fFont.getWidths(glyphs, count, widths)` 编译失败，提示 `no matching function`。

**根因**：m150 的 `SkFont::getWidths` 签名从旧版 3-arg `(const SkGlyphID*, int, float*)` 改为 2-arg `SkSpan` 版本：
```cpp
void getWidths(SkSpan<const SkGlyphID> glyphs, SkSpan<SkScalar> widths) const;
```

**修复**：
```cpp
info.fFont.getWidths(
    SkSpan<const SkGlyphID>{fCurrentBuffer.glyphs, (size_t)info.glyphCount},
    SkSpan<SkScalar>{adv.data(), (size_t)info.glyphCount});
```

---

## 13. hit_test 末位字形右半边应返回 text_len（光标定位 bug）

**现象**：点击最后一个字形的右半边，hit_test 返回该字形的 cluster 值而非 text_len（行尾偏移），导致无法在行尾放置光标。

**根因**：原实现中 `if (x > mid && i + 1 < glyph_count)` 对最后一个字形短路——`i + 1 < glyph_count` 为 false，右半边也返回 `cluster[i]`。

**修复**：
```cpp
if (x > mid) {
    if (i + 1 < blob->glyph_count)
        return blob->cluster_indices[i + 1];
    else
        return (int)blob->text_len;  // EOL cursor
}
```

同时需在 `CevgTextBlob_` 中添加 `text_len` 字段，在 `cevg_blob_from_result` 时填入源串字节长度。

---

## 14. advance 不应从位置差重建（末位膨胀 + BiDi 不可靠）

**现象**：末位字形 advance = `width - positions_x[last]`，继承了 width 的偏大。跨 BiDi run 边界时视觉序位置会跳，位置差也不可靠。

**根因**：用 `positions_x[i+1] - positions_x[i]` 重建 advance 是错误的——positions 包含 kerning/GPOS 调整，而 advance 是字形的名义步进宽度。两者是不同的概念。

**修复**：从 `SkFont::getWidths` 逐 run 取真实 advance（见 #11），直接拷贝到 blob 中，不再从位置差重建。

---

## 15. SkTextBlobBuilderRunHandler offset 决定基线约定（零开销修复）

**现象**：`draw_text_blob(x, y)` 中 y 被当作文本顶部而非基线，与 Skia 标准约定不一致。

**根因**：`SkTextBlobBuilderRunHandler` 初始化时 offset 传了 `{0, 0}`，但 `commitRunInfo()` 内部会做 `fCurrentPosition.fY -= fMaxRunAscent`，把 |ascent| 嵌入了字形位置。结果 blob 坐标从 ascent 线开始而非基线。

**修复**：传 `{0, fm.fAscent}` 作为 handler offset（ascent 为负数），与 `commitRunInfo` 的偏移相消，使 blob 坐标基线相对。零运行时开销。

---

## 移植检查清单

- [ ] 对照 Skia GN `defines` 列表，确保所有 ICU/HarfBuzz/FontConfig 宏一致
- [ ] 检查 `SkFontMgr_New_FontConfig` 函数签名是否变化
- [ ] 检查 `icudtl.dat` 中的符号名（`icudt74_dat` → `icudt7X_dat`）
- [ ] `.S` 文件中 `.balign 16` 不能删
- [ ] `U_ENABLE_DYLOAD=0` 必须定义
- [ ] `ICU_VERSION` 与 `uvernum.h` 一致
- [ ] 自定义 RunHandler 的 buffer 必须与内部 handler 共享
- [ ] expat 编译定义与 Skia GN 一致
- [ ] `SkTextBlob::bounds()` 不能当宽度用——它是保守墨迹盒
- [ ] `SkFont::getWidths` 在 m150 中是 SkSpan 2-arg 版本
- [ ] advance 从 `SkFont::getWidths` 取，不从位置差重建
- [ ] RunHandler offset 传 `{0, ascent}` 使 blob 坐标基线相对
- [ ] hit_test 末位右半边返回 `text_len`（行尾光标）
- [ ] `CevgTextBlob_` 需存 `text_len` 和 `advances` 字段
