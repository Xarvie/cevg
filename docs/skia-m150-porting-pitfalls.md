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

## 移植检查清单

- [ ] 对照 Skia GN `defines` 列表，确保所有 ICU/HarfBuzz/FontConfig 宏一致
- [ ] 检查 `SkFontMgr_New_FontConfig` 函数签名是否变化
- [ ] 检查 `icudtl.dat` 中的符号名（`icudt74_dat` → `icudt7X_dat`）
- [ ] `.S` 文件中 `.balign 16` 不能删
- [ ] `U_ENABLE_DYLOAD=0` 必须定义
- [ ] `ICU_VERSION` 与 `uvernum.h` 一致
- [ ] 自定义 RunHandler 的 buffer 必须与内部 handler 共享
- [ ] expat 编译定义与 Skia GN 一致
