# Cevg 项目技术文档

> 面向接手开发者（Linux / 跨平台）的快速入门指南。

## 0. CPU-only 快速入门（无 GPU 环境）

如果你在没有 Vulkan 驱动/GPU 的环境（如 CI、远程服务器）工作，只需：

```bash
cmake -B build -G Ninja -DCEVG_VULKAN=OFF -DCEVG_BUILD_TESTS=OFF
cmake --build build
```

此模式下：
- **不需要** Vulkan SDK、vulkan-1.lib、libvulkan.so、GPU 驱动
- **不需要** 编译 glad2（cevg_glad 目标不创建）
- Skia 只编译 CPU 光栅化路径，Vulkan 后端代码完全不参与编译
- 公共 API 中 Vulkan 相关函数（`cevg_gpu_device_create_vulkan` 等）不可见
- 使用 `cevg_gpu_device_create_cpu()` 或 `cevg_gpu_device_create_auto()` 创建设备

如需启用 Vulkan 后端，见第 4 节。

## 1. 项目概览

**cevg** 是一个矢量图形渲染库（v0.2.0），提供 C ABI 公共接口。核心特点：

- 基于 **Skia Graphite**（Skia 新一代 GPU 后端）实现
- **Vulkan** 作为可选 GPU 后端，通过 **glad2** 运行时动态加载
- **编译时零 Vulkan SDK 依赖** — 不需要安装 Vulkan SDK，不需要 vulkan-1.lib
- GPU 直出呈现（Vulkan swapchain，零 CPU 回读）
- 严格的 ABI 稳定性契约（版本化结构体、步进数组、后端不透明化）
- 多 Recorder 多线程录制 + 保留式 Display List

## 2. 目录结构

```
cevg/
├── CMakeLists.txt              # 主构建脚本
├── include/cevg/cevg.h         # 公共 C ABI 头文件（唯一公共头文件）
├── common/
│   ├── cevg_internal.h         # 内部共享结构体（Path/Paint）
│   └── cevg_common.c           # 后端无关的 Path/Paint 实现（纯 C11）
├── src/
│   └── cevg_skia_graphite.cpp  # Graphite/Vulkan 后端实现（~3000行）
├── third/
│   ├── glad/                   # glad2 Vulkan 运行时加载器
│   │   ├── include/glad/vulkan.h       # glad2 生成的主头文件
│   │   ├── include/vulkan/             # Header Hijacking 伪装头文件
│   │   │   ├── vulkan.h                #   → #include <glad/vulkan.h>
│   │   │   ├── vulkan_core.h           #   → #include <glad/vulkan.h> + 兼容层
│   │   │   └── vulkan_win32.h          #   → #include <glad/vulkan.h>
│   │   ├── src/vulkan.c                # glad2 加载器实现
│   │   └── GLAD_VULKAN_README.txt      # glad2 详细配置指南
│   ├── skia/
│   │   ├── CMakeLists.txt              # Skia 源码构建脚本
│   │   ├── skia-m150/                  # Skia m150 完整源码
│   │   └── third/                      # Skia 第三方依赖（expat, harfbuzz, icu, zlib）
│   └── vulkanmemoryallocator/          # VMA（Vulkan Memory Allocator）
└── tests/                      # 测试（可选，-DCEVG_BUILD_TESTS=ON）
```

## 3. 构建系统

### 3.1 CMake 选项

| 选项 | 默认 | 说明 |
|---|---|---|
| `CEVG_SHARED` | OFF | 构建为共享库（DLL/SO） |
| `CEVG_VULKAN` | ON | 启用 Vulkan GPU 后端（glad2 运行时加载） |
| `CEVG_STRICT_WARNINGS` | ON | 启用 -Wall -Wextra (/W4) |
| `CEVG_WARNINGS_AS_ERRORS` | OFF | 警告视为错误 |
| `CEVG_SANITIZE_THREAD` | OFF | ThreadSanitizer |
| `CEVG_BUILD_TESTS` | OFF | 构建测试 |

### 3.2 构建命令

```bash
# Vulkan + CPU 后端（默认）
cmake -B build -G Ninja -DCEVG_VULKAN=ON
cmake --build build

# 纯 CPU 后端（零 Vulkan 依赖）
cmake -B build -G Ninja -DCEVG_VULKAN=OFF
cmake --build build
```

### 3.3 依赖关系图

```
cevg (SHARED/STATIC)
 ├── PRIVATE → skia (STATIC, add_subdirectory)
 │              ├── icu, harfbuzz, expat, zlib (Skia 第三方)
 │              └── [CEVG_VULKAN=ON] → cevg_glad (PUBLIC via skia)
 └── PRIVATE → cevg_glad (STATIC, 仅 CEVG_VULKAN=ON)
      └── [UNIX] → ${CMAKE_DL_LIBS} (dlclose/dlopen)

cevg_common (OBJECT) ← 无 Skia 依赖，纯 C11
```

### 3.4 语言标准

- C11（cevg_common.c，MSVC 需要 `/experimental:c11atomics`）
- C++20（cevg_skia_graphite.cpp + Skia）

## 4. Vulkan 集成架构（核心重点）

### 4.1 设计目标

**编译时零 Vulkan SDK 依赖，运行时动态加载。** 用户不需要安装 Vulkan SDK，不需要链接 vulkan-1.lib / libvulkan.so。glad2 在运行时通过 `LoadLibraryA("vulkan-1.dll")` / `dlopen("libvulkan.so")` 动态加载 Vulkan 驱动。

### 4.2 glad2 配置

由 [gen.glad.sh](https://gen.glad.sh/) 生成，采用以下配置：

- **Generator**: C/C++
- **API**: Vulkan 1.3（向下兼容 1.0, 1.1, 1.2）
- **Options**:
  - [开启] **loader** — 内置动态库查找，生成 `gladLoaderLoadVulkan`
  - [开启] **on-demand** — 按需延迟加载函数指针，允许扩展在特定平台残缺（如 Linux 下缺 Win32 符号），防止启动时硬闪退
  - [关闭] 其他选项（header only, debug, mx 等均关闭）
- **不要修改 glad2 自动生成的代码**（`include/glad/vulkan.h` 和 `src/vulkan.c`）

**64 个扩展** — 覆盖 Skia m150 全部需求：

| 扩展 | 说明 |
|---|---|
| VK_KHR_surface | 窗口系统总抽象 |
| VK_KHR_swapchain | 交换链（画面显示必备） |
| VK_KHR_portability_enumeration | Apple 平台必须：允许非原生驱动（MoltenVK） |
| VK_KHR_get_physical_device_properties2 | Apple 平台必须：获取特殊硬件属性 |
| VK_KHR_win32_surface | 平台原生支持：Windows |
| VK_KHR_android_surface | 平台原生支持：Android |
| VK_KHR_wayland_surface | 平台原生支持：Linux 现代桌面 |
| VK_KHR_xcb_surface | 平台原生支持：Linux 传统桌面 / Wine 兼容 |
| VK_KHR_xlib_surface | 平台原生支持：Linux 传统桌面 / Wine 兼容 |
| VK_EXT_rasterization_order_attachment_access | 光栅化顺序附件访问 |
| VK_ARM_rasterization_order_attachment_access | ARM 版光栅化顺序附件访问 |
| VK_EXT_blend_operation_advanced | 高级混合操作 |
| VK_EXT_extended_dynamic_state | 扩展动态状态（→Vulkan 1.3） |
| VK_EXT_extended_dynamic_state2 | 扩展动态状态2（→Vulkan 1.3） |
| VK_EXT_vertex_input_dynamic_state | 顶点输入动态状态 |
| VK_EXT_graphics_pipeline_library | 图形管线库 |
| VK_KHR_sampler_ycbcr_conversion | YCbCr 采样器转换（→Vulkan 1.1） |
| VK_EXT_rgba10x6_formats | RGBA10x6 格式 |
| VK_KHR_synchronization2 | 同步2（→Vulkan 1.3） |
| VK_KHR_dynamic_rendering | 动态渲染（→Vulkan 1.3） |
| VK_KHR_dynamic_rendering_local_read | 动态渲染本地读取（→Vulkan 1.4） |
| VK_EXT_multisampled_render_to_single_sampled | 多采样渲染到单采样 |
| VK_EXT_host_image_copy | 主机图像拷贝（→Vulkan 1.4） |
| VK_EXT_pipeline_creation_cache_control | 管线创建缓存控制（→Vulkan 1.3） |
| VK_EXT_frame_boundary | 帧边界 |
| VK_EXT_device_fault | 设备故障诊断 |
| VK_KHR_create_renderpass2 | RenderPass2 创建（→Vulkan 1.2） |
| VK_EXT_load_store_op_none | 无操作 load/store |
| VK_KHR_load_store_op_none | 无操作 load/store（→Vulkan 1.4） |
| VK_EXT_conservative_rasterization | 保守光栅化 |
| VK_KHR_driver_properties | 驱动属性查询（→Vulkan 1.2） |
| VK_KHR_push_descriptor | 推送描述符（→Vulkan 1.4） |
| VK_KHR_pipeline_library | 管线库 |
| VK_KHR_copy_commands2 | 拷贝命令2 |
| VK_KHR_format_feature_flags2 | 格式特性标志2（→Vulkan 1.3） |
| VK_KHR_depth_stencil_resolve | 深度模板解析（→Vulkan 1.2） |
| VK_KHR_shader_draw_parameters | 着色器绘制参数（→Vulkan 1.1） |
| VK_KHR_draw_indirect_count | 间接绘制计数（→Vulkan 1.2） |
| VK_KHR_sampler_mirror_clamp_to_edge | 采样器镜像夹紧到边（→Vulkan 1.2） |
| VK_EXT_descriptor_indexing | 描述符索引（→Vulkan 1.2） |
| VK_EXT_sampler_filter_minmax | 采样器过滤最小最大（→Vulkan 1.2） |
| VK_EXT_shader_viewport_index_layer | 着色器视口索引层（→Vulkan 1.2） |
| VK_ANDROID_external_memory_android_hardware_buffer | Android 硬件缓冲区 |
| VK_KHR_dedicated_allocation | 专用分配（→Vulkan 1.1） |
| VK_KHR_get_memory_requirements2 | 内存需求查询2（→Vulkan 1.1） |
| VK_KHR_bind_memory2 | 绑定内存2（→Vulkan 1.1） |
| VK_KHR_external_memory | 外部内存 |
| VK_KHR_external_memory_capabilities | 外部内存能力 |
| VK_KHR_device_group | 设备组 |
| VK_KHR_buffer_device_address | 缓冲区设备地址（→Vulkan 1.2） |
| VK_EXT_filter_cubic | 三次过滤 |
| VK_EXT_memory_budget | 内存预算 |
| VK_EXT_memory_priority | 内存优先级 |
| VK_KHR_global_priority | 全局优先级 |
| VK_KHR_shader_subgroup_rotate | 子组旋转 |
| VK_KHR_shader_float_controls2 | 浮点控制2 |
| VK_KHR_shader_expect_assume | expect/assume |
| VK_EXT_line_rasterization | 线段光栅化 |
| VK_EXT_vertex_attribute_divisor | 顶点属性除数 |
| VK_EXT_index_type_uint8 | uint8 索引类型 |
| VK_KHR_maintenance5 | 维护5 |
| VK_KHR_maintenance6 | 维护6 |
| VK_EXT_pipeline_protected_access | 管线保护访问 |
| VK_EXT_pipeline_robustness | 管线鲁棒性 |
| VK_EXT_queue_family_foreign | 外部队列族 |

### 4.3 Header Hijacking（头文件劫持）

Skia 源码使用 `#include <vulkan/vulkan.h>` 等标准 Vulkan 头文件路径。我们**不修改 Skia 源码**，而是在 `third/glad/include/vulkan/` 下放置伪装头文件，拦截这些 include 并重定向到 glad2：

```
third/glad/include/vulkan/vulkan.h       → #include <glad/vulkan.h>
third/glad/include/vulkan/vulkan_core.h  → #include <glad/vulkan.h> + Vulkan 1.4 兼容层
third/glad/include/vulkan/vulkan_win32.h → #include <glad/vulkan.h>
```

CMake 通过 include 路径优先级让 `third/glad/include` 排在 Skia 内部 Vulkan 路径之前，实现劫持。

**关键设计**：伪装头文件**不设置** `VULKAN_H_` / `VULKAN_CORE_H_` 守卫宏，因为 glad2 自己会设置这些宏并有 `GLAD_VULKAN_H_` 防止重复包含。如果伪装头文件也设置守卫宏，会导致 glad2 的 `#error "header already included"` 报错。

### 4.4 Vulkan 1.4 兼容层

glad2 生成的是 Vulkan 1.3，但 Skia m150 使用了部分 Vulkan 1.4 内容。兼容层在 `vulkan_core.h` 中补充：

| 缺失项 | 处理方式 |
|---|---|
| `VK_API_VERSION_1_4` | `#define VK_API_VERSION_1_4 VK_MAKE_API_VERSION(0, 1, 4, 0)` |
| `VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES` | `#define ... ((VkStructureType)55)` — 值 55 是 Vulkan 核心枚举值，不是扩展值（注意：不是 1000470000，那是 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES） |
| `VkPhysicalDeviceVulkan14Features` | 手动定义结构体（22 个 feature 字段） |
| `PFN_vkTransitionImageLayout` | `typedef PFN_vkTransitionImageLayoutEXT PFN_vkTransitionImageLayout` |
| `PFN_vkCopyMemoryToImage` | `typedef PFN_vkCopyMemoryToImageEXT PFN_vkCopyMemoryToImage` |
| `PFN_vkCopyImageToMemory` | `typedef PFN_vkCopyImageToMemoryEXT PFN_vkCopyImageToMemory` |

### 4.5 Skia 的 Vulkan 桥接

Skia 不直接调用全局 Vulkan 函数，而是通过函数指针表（`GrVkInterface` / `GetProc` 回调）。cevg 通过 `backendCtx.fGetProc` 桥接注入：

```cpp
// cevg_skia_graphite.cpp
d->getInstanceProc = gipa
    ? reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa)
    : vkGetInstanceProcAddr;  // glad2 on-demand: auto-loads from vulkan-1.dll
```

### 4.6 已关闭的 Skia 内部 VK 头文件

`SK_USE_INTERNAL_VULKAN_HEADERS` 宏已移除，Skia 的 `include/third_party/vulkan/` 目录已删除。Skia 的 `SkiaVulkan.h` 现在走 `#include <vulkan/vulkan_core.h>`，被我们的伪装头文件拦截。

### 4.7 VMA 也走 glad2

`vk_mem_alloc.h` 的 `#include <vulkan/vulkan.h>` 同样被 Header Hijacking 拦截，无需额外处理。

## 5. Vulkan 职责划分与 5 大平台运行时策略

### 5.1 核心职责划分

cevg 是一个**被动消费** Vulkan 的库：它不创建、不拥有、不销毁任何 Vulkan 对象。
所有 Vulkan 生命周期管理由 App 层负责，cevg 只消费 App 传入的句柄。

| 职责 | App（调用方） | cevg（库） |
|---|:---:|:---:|
| 加载 Vulkan 动态库（vulkan-1.dll / libvulkan.so / MoltenVK） | **负责** | 不管 |
| 创建 VkInstance | **负责** | 不管 |
| 枚举/选择 VkPhysicalDevice | **负责** | 不管 |
| 创建 VkDevice + 启用所需扩展 | **负责** | 不管 |
| 获取 VkQueue + queue family index | **负责** | 不管 |
| 提供 `vkGetInstanceProcAddr` 函数指针 | **负责** | 接收 |
| Vulkan 函数加载（glad2 on-demand） | 不需要 | **内部负责** |
| Vulkan 对象生命周期管理（销毁 Instance/Device） | **负责** | 不管 |
| 渲染资源管理（Surface/Canvas/Paint 等） | 通过 cevg API | **负责** |

**设计原则**：
1. **单一所有权**：谁创建谁销毁。App 创建的 Vulkan 对象由 App 销毁，cevg 创建的渲染对象由 cevg 销毁。
2. **glad2 封装在库内部**：App 不需要链接或使用 glad2。cevg 在 `cevg_gpu_device_create_vulkan` 中调用 `gladLoaderSetVulkanInstance/Device` 初始化内部 on-demand 加载，确保所有 `vk*` 函数指针可用。
3. **App 只需提供原始句柄**：通过 `CevgVulkanDevice` 结构体传入 void* 句柄 + `vkGetInstanceProcAddr`，不需要任何 Vulkan 头文件或加载器。

### 5.2 App 集成代码模板

```c
#include <cevg/cevg.h>

/* ---- App 层：Vulkan 引导（无需 glad2，无需 Vulkan SDK） ---- */

/* 1. 加载 vulkan 动态库，获取 vkGetInstanceProcAddr */
/*    Windows: GetProcAddress(LoadLibraryA("vulkan-1.dll"), "vkGetInstanceProcAddr") */
/*    Linux:   dlsym(dlopen("libvulkan.so.1", ...), "vkGetInstanceProcAddr") */
/*    macOS:   dlsym(dlopen("@rpath/libMoltenVK.dylib", ...), "vkGetInstanceProcAddr") */

/* 2. 通过 PFN_vk* 原始函数指针创建 Instance / Device / Queue */
/*    PFN_vkCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance"); */
/*    ... */

/* 3. 填充 CevgVulkanDevice 传给 cevg */
PFN_vkGetInstanceProcAddr gipa = /* 从步骤1获取 */;

CevgVulkanDevice vk_dev = {
    .struct_size            = sizeof(CevgVulkanDevice),
    .vk_instance            = my_instance,
    .vk_physical_device     = my_physical_device,
    .vk_device              = my_device,
    .vk_queue               = my_queue,
    .vk_queue_index         = my_queue_family_index,
    .enabled_ext_names      = my_enabled_ext_names,
    .enabled_ext_count      = my_enabled_ext_count,
    .vk_get_instance_proc_addr = (void*)gipa,   /* 可选但推荐 */
};

/* 4. 创建 cevg 设备和上下文 */
CevgGpuDevice* gpu = cevg_gpu_device_create_vulkan(&vk_dev);
CevgConfig cfg = {
    .struct_size  = sizeof(CevgConfig),
    .device       = gpu,
    .color_space  = kCevgColorSpace_sRGB,
};
CevgContext* ctx = cevg_context_create(&cfg);

/* 5. 正常使用 cevg API 渲染... */

/* 6. 销毁顺序：先 cevg，后 Vulkan */
cevg_context_destroy(ctx);
cevg_gpu_device_destroy(gpu);
/* vkDestroyDevice, vkDestroyInstance ... */
```

### 5.3 cevg 内部 Vulkan 加载流程

```
App 调用 cevg_gpu_device_create_vulkan(&vk_dev)
  │
  ├─ 保存 VkInstance/VkDevice/VkQueue 等句柄
  │
  ├─ gladLoaderSetVulkanInstance(instance)   ← 告诉 glad2 用哪个 Instance
  ├─ gladLoaderSetVulkanDevice(device)       ← 告诉 glad2 用哪个 Device
  │
  └─ 保存 vkGetInstanceProcAddr（来自 vk_dev 或 glad2 内部）
     │
     └─ 后续所有 vk* 调用通过 glad2 on-demand 自动加载
        ├─ 实例级函数 → vkGetInstanceProcAddr(instance, name)
        ├─ 设备级函数 → vkGetDeviceProcAddr(device, name)  ← 更快
        └─ 兜底       → dlsym(vulkan-1.dll, name)
```

### 5.4 五大平台运行时策略

#### Windows

| 项目 | 说明 |
|---|---|
| 运行时依赖 | 显卡驱动自带 `vulkan-1.dll`（System32 或驱动目录） |
| App 加载方式 | `LoadLibraryA("vulkan-1.dll")` + `GetProcAddress` |
| 实例扩展 | `VK_KHR_surface` + `VK_KHR_win32_surface` |
| 打包 | 无需额外分发，驱动安装时自动注册 |
| 注意 | 无需 Vulkan SDK，无需 vulkan-1.lib |

#### Linux（X11 / Wayland）

| 项目 | 说明 |
|---|---|
| 运行时依赖 | 系统/驱动自带 `libvulkan.so.1`（mesa / NVIDIA 驱动） |
| App 加载方式 | `dlopen("libvulkan.so.1", RTLD_NOW)` + `dlsym` |
| 实例扩展 | `VK_KHR_surface` + `VK_KHR_xcb_surface` 或 `VK_KHR_wayland_surface` |
| 打包 | 纯源码编译，需安装 mesa-vulkan-drivers 或 NVIDIA 驱动 |
| 注意 | 链接 `${CMAKE_DL_LIBS}`（已在 CMakeLists.txt 中处理） |

#### Android

| 项目 | 说明 |
|---|---|
| 运行时依赖 | 系统自带 `libvulkan.so`（NDK 自带头文件和 stub） |
| App 加载方式 | NDK 链接 `-lvulkan`，或 `dlopen("libvulkan.so", ...)` + `dlsym` |
| 实例扩展 | `VK_KHR_surface` + `VK_KHR_android_surface` |
| 打包 | NDK 原生支持，无需额外分发 |
| 注意 | Android NDK 提供 `libvulkan.so` stub，可直接链接 |

#### macOS

| 项目 | 说明 |
|---|---|
| 运行时依赖 | App 内嵌 `libMoltenVK.dylib`（Vulkan → Metal 翻译层） |
| App 加载方式 | `dlopen("@rpath/libMoltenVK.dylib", RTLD_NOW)` + `dlsym` |
| 实例扩展 | `VK_KHR_surface` + `VK_EXT_metal_surface` + **`VK_KHR_portability_enumeration`** |
| 打包 | Xcode "Embed & Sign" 嵌入 MoltenVK dylib |
| 注意 | 创建 Instance **必须**加 `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` flag，否则枚举不到设备 |

#### iOS

| 项目 | 说明 |
|---|---|
| 运行时依赖 | App 内嵌 `libMoltenVK.dylib` 或静态链接 MoltenVK `.a` |
| App 加载方式 | 静态链接时直接调用；动态链接时 `dlopen("@rpath/libMoltenVK.dylib", ...)` + `dlsym` |
| 实例扩展 | `VK_KHR_surface` + `VK_EXT_metal_surface` + **`VK_KHR_portability_enumeration`** |
| 打包 | Xcode "Embed & Sign" 嵌入 MoltenVK dylib，或静态链接 |
| 注意 | 同 macOS，必须加 portability flag；iOS 不支持动态库热加载，推荐静态链接 |

### 5.5 Apple 平台特殊处理

macOS / iOS 使用 MoltenVK（Vulkan → Metal 翻译层），有两处必须特殊处理：

```c
/* 1. 创建 Instance 时必须加 portability flag 和扩展 */
VkInstanceCreateInfo createInfo = {
    .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,  /* 必须！ */
    .ppEnabledExtensionNames = (const char*[]){
        "VK_KHR_portability_enumeration",     /* 必须！ */
        "VK_KHR_get_physical_device_properties2", /* 必须！ */
        "VK_KHR_surface",
        "VK_EXT_metal_surface",
        NULL
    },
    .enabledExtensionCount = 4,
};

/* 2. MoltenVK 不支持所有 Vulkan 扩展，启用设备扩展前检查可用性 */
```

### 5.6 Skia Graphite 所需设备扩展

Skia Graphite 要求 App 启用尽可能多的设备扩展。推荐做法：枚举物理设备的所有可用扩展并全部启用（测试代码中的 `vulkan_setup.c` 就是这么做的）。生产环境中可以只启用 Skia 需要的扩展子集。

## 6. CEVG_VULKAN 的完整影响

| 方面 | CEVG_VULKAN=ON | CEVG_VULKAN=OFF |
|---|---|---|
| cevg_glad 静态库 | 创建 | 不创建 |
| Skia 配置 | `SKIA_USE_VULKAN=ON` | `SKIA_USE_VULKAN=OFF` |
| 编译定义 | `CEVG_VULKAN` 宏 | 无此宏 |
| 链接 | cevg → cevg_glad | 无 VK 链接 |
| 公共 API | 暴露 Vulkan 相关 API | Vulkan API 不可见 |
| .def 导出 | 包含 6 个 VK 专属符号 | 不包含 |
| 运行时 | GPU 直出（Vulkan swapchain） | 纯 CPU 光栅化 |

Vulkan 专属公共 API（仅 `CEVG_VULKAN=ON` 时可见）：
- `cevg_gpu_device_create_vulkan`
- `cevg_surface_create_for_vk_surface`
- `cevg_gpu_profiler_enable / begin_frame / end_frame / get_frame_time`

## 7. 代码分层

```
┌─────────────────────────────────────────────────────┐
│  公共层  include/cevg/cevg.h                        │
│  纯 C ABI，后端不透明，版本化结构体                    │
├─────────────────────────────────────────────────────┤
│  共享层  common/cevg_internal.h + cevg_common.c      │
│  Path/Paint 的后端无关实现，纯 C11，无 Skia 依赖      │
├─────────────────────────────────────────────────────┤
│  后端层  src/cevg_skia_graphite.cpp                  │
│  Skia Graphite + Vulkan 完整绑定                     │
├─────────────────────────────────────────────────────┤
│  第三方层  third/                                    │
│  Skia m150 源码构建 + glad2 + VMA                    │
└─────────────────────────────────────────────────────┘
```

## 8. 注意事项

1. **不要修改 glad2 自动生成的代码**（`include/glad/vulkan.h` 和 `src/vulkan.c`）。如需补充类型或宏，在 `include/vulkan/vulkan_core.h` 兼容层中添加。

2. **不要在 CMake 中链接官方 Vulkan 库**。Windows/Linux/Android 完全由 glad2 动态加载；Apple 端配置好 MoltenVK 嵌入即可。

3. **不要定义 `SK_USE_INTERNAL_VULKAN_HEADERS`**。Skia 的内部 VK 头文件已删除，所有 VK 头文件走 glad2 伪装头文件。

4. **函数调用前做 Null 检查**：on-demand 模式下，平台特异扩展的函数指针可能为 NULL（如 Linux 下无 Win32 surface 函数），调用前检查 `if (vkCreateWin32SurfaceKHR != nullptr)`。

5. **MSVC 特殊处理**：C11 原子操作需要 `/experimental:c11atomics` 编译选项（已在 CMakeLists.txt 中配置）。

6. **Linux 构建需要**：`libdl`（已通过 `${CMAKE_DL_LIBS}` 自动链接）、C++20 编译器、CMake 3.20+。
