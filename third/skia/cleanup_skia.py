"""
Skia 目录清理脚本（执行版）
==========================
用于删除 Skia 源码树中 CMake 编译不需要的目录和文件。
仅保留核心源码、必要头文件和所需的三方依赖。

默认行为: 同时清理 Skia 源码和三方库 (third/)。

使用方式:
    python cleanup_skia.py                          # 直接执行清理 (Skia + third)
    python cleanup_skia.py --preview                # 只预览，不实际删除
    python cleanup_skia.py --skip-third             # 只清理 Skia 源码，跳过第三方库
    python cleanup_skia.py --skia-dir <路径>        # 指定 Skia 目录
    python cleanup_skia.py --third-dir <路径>       # 指定三方库目录 (默认 <skia>/../third)
"""

import argparse
import os
import shutil
import sys


def parse_args():
    parser = argparse.ArgumentParser(description="清理 Skia 中 CMake 不需要的目录 (默认: 执行模式 + 清理三方库)")
    parser.add_argument(
        "--skia-dir",
        default=None,
        help="Skia 根目录路径。默认自动查找脚本所在目录或当前目录的 src/。",
    )
    parser.add_argument(
        "--third-dir",
        default=None,
        help="三方库根目录 (expat/harfbuzz/icu 所在的目录)。默认 <skia-dir>/../third。",
    )
    parser.add_argument(
        "--skip-third",
        action="store_true",
        help="跳过三方库目录的清理。",
    )
    parser.add_argument(
        "--preview",
        action="store_true",
        help="只预览要删除的内容，不实际执行。",
    )
    return parser.parse_args()


def find_skia_root(sk_dir: str | None) -> str:
    """定位 Skia 根目录"""
    if sk_dir:
        path = os.path.abspath(sk_dir)
        if not os.path.isdir(path):
            sys.exit(f"错误: 指定的目录不存在: {path}")
        return path

    script_dir = os.path.dirname(os.path.abspath(__file__))
    for candidate in [script_dir, os.getcwd()]:
        if os.path.isdir(os.path.join(candidate, "src")):
            return candidate

    sys.exit("错误: 找不到 Skia 目录（未找到 src/ 子目录）。请用 --skia-dir 指定。")


def resolve_third_dir(skia_root: str, third_dir_arg: str | None) -> str | None:
    """定位三方库根目录。找不到则返回 None。"""
    if third_dir_arg:
        path = os.path.abspath(third_dir_arg)
        if not os.path.isdir(path):
            sys.exit(f"错误: 指定的三方库目录不存在: {path}")
        return path

    default = os.path.normpath(os.path.join(skia_root, "..", "third"))
    if os.path.isdir(default):
        return default
    return None


def remove_item(path: str, execute: bool) -> None:
    """删除文件或目录（预览或实际执行）"""
    if not os.path.exists(path):
        return

    label = "删除" if execute else "发现"
    if os.path.isdir(path):
        size = _dir_size(path)
        print(f"  [{label}] 目录: {path}  ({_fmt_size(size)})")
        if execute:
            shutil.rmtree(path, ignore_errors=True)
    else:
        size = os.path.getsize(path)
        print(f"  [{label}] 文件: {path}  ({_fmt_size(size)})")
        if execute:
            os.remove(path)


def _dir_size(path: str) -> int:
    total = 0
    for dirpath, _, filenames in os.walk(path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            try:
                total += os.path.getsize(fp)
            except OSError:
                pass
    return total


def _fmt_size(bytes_: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if bytes_ < 1024:
            return f"{bytes_:.1f}{unit}"
        bytes_ /= 1024
    return f"{bytes_:.1f}TB"


def remove_specific_subdirs(parent: str, subdirs: list[str], execute: bool, header: str | None = None) -> None:
    """删除 parent 下的若干子目录（仅当存在时）。"""
    if not os.path.isdir(parent):
        return
    if header:
        print(f"--- {header} ---")
    for d in subdirs:
        path = os.path.join(parent, d)
        remove_item(path, execute)


# =============================================================
# Skia 源码清理
# =============================================================
def cleanup_skia(root: str, execute: bool) -> None:
    """Skia 源码树 (skia-m150/) 的清理。"""

    # ---- 1. 根目录下一级目录 ----
    # 一类零风险新增：platform_tools (Android 调试/sample 工具),
    #                agents (Claude skills 残留),
    #                client_utils (Android client utils, 未被 CMake 引用)
    root_dirs_to_delete = [
        "agents",
        "bazel",
        "bench",
        "bin",
        "build_overrides",
        "client_utils",
        "demos.skia.org",
        "dm",
        "docs",
        "example",
        "experimental",
        "fuzz",
        "gm",
        "gn",
        "infra",
        "platform_tools",
        "relnotes",
        "resources",
        "rust",
        "site",
        "specs",
        "tests",
        "toolchain",
        "tools",
        "modules",  # 下面会单独处理，只保留部分
    ]

    # ---- 2. third_party 下保留的目录（其它全部删除）----
    # 一类零风险：之前的 expat/harfbuzz/icu/zlib 都只是 GN 残留，
    #            CMake 实际只把它们当作 include 搜索路径用，全部可以清掉。
    third_party_keep: set[str] = set()

    # ---- 3. modules 下保留的目录（其它全部删除）----
    # 一类零风险：skparagraph 在 CMakeLists 中无任何引用 (tests/src/slides 都无),
    #            从保留白名单中移除。
    modules_keep = {
        "skcms",
        "skshaper",
        "skunicode",
    }

    # ---- 4. 根目录下需要删除的文件 ----
    # 一类零风险新增：Google 仓库残留文件
    root_files_to_delete = [
        # Bazel 相关
        ".bazelignore",
        ".bazeliskrc",
        ".bazelproject",
        ".bazelrc",
        ".bazelversion",
        "BUILD.bazel",
        "MODULE.bazel",
        "MODULE.bazel.lock",
        "WORKSPACE.bazel",
        # GN 相关
        ".gn",
        "BUILD.gn",
        # 其他构建/CI 无关文件
        ".vpython3",
        "CQ_COMMITTERS",
        "DEPS",
        "DIR_METADATA",
        "OWNERS",
        "OWNERS.android",
        "PRESUBMIT.py",
        "PRESUBMIT_test.py",
        "PRESUBMIT_test_mocks.py",
        "RELEASE_NOTES.md",
        "codereview.settings",
        "go.mod",
        "go.sum",
        "libskia_blocklist.txt",
        "package-lock.json",
        "package.json",
        "requirements.txt",
        "serve-bazel-test-undeclared-outputs.sh",
        "tools.go",
        "whitespace.txt",
    ]

    # ---- 5. src/ 下一类零风险可清子目录（CMake 0 引用）----
    # 注意: src/android 不能删，GrCanvas.cpp 等源文件 #include 了 android 头文件，
    #       编译时需要这些源文件和头文件存在。
    src_subdirs_to_delete = [
        # "android",   # 保留 - 多个 .cpp 引用 include/android/ 头文件
        "pdf",       # SkPDF*.cpp - 未被 CMake 编译
        "svg",       # SVG canvas - 未被 CMake 编译
        "xps",       # XPS document - 未被 CMake 编译
    ]

    # ---- 6. include/ 下一类零风险可清子目录（CMake 0 引用）----
    # 注意: include/android 和 include/docs 不能删，多个源文件 #include 了这些头文件。
    include_subdirs_to_delete = [
        # "android",   # 保留 - GrCanvas.cpp, SkImage_RasterPinnable.cpp 等引用
        # "docs",      # 保留 - SkMultiPictureDocument.cpp 引用 SkMultiPictureDocument.h
        "svg",
        "xps",
    ]

    # ---- 7. src/gpu/graphite/ 下 CMake 显式排除的子目录 ----
    # CMakeLists.txt 中通过 list(FILTER ... EXCLUDE REGEX) 排除了以下子目录:
    #   /Android/  /dawn/  /mtl/  /precompile/  /sparse_strips/  /vk/  /Vello/
    graphite_subdirs_to_delete = [
        "Android",
        "Vello",
        "dawn",
        "mtl",
        # "precompile",  # 保留 - PublicPrecompile.cpp 引用 precompile/ 头文件
        "sparse_strips",
        # "vk",  # 保留 - Graphite VK 后端源码，CEVG_VULKAN=ON 时需要
    ]

    action = "执行" if execute else "预览"
    print(f"{'=' * 60}")
    print(f"  Skia 目录清理 - {action}模式")
    print(f"  目标: {root}")
    print(f"{'=' * 60}\n")

    # ---- 执行删除 ----
    print("--- 根目录下的一级子目录 ---")
    for d in root_dirs_to_delete:
        path = os.path.join(root, d)
        if d == "modules":
            # modules 要保留部分子目录，不能整体删除
            continue
        remove_item(path, execute)

    # ---- 清理 third_party ----
    print("\n--- third_party/ 三方依赖 (含 GN 残留) ---")
    tp_dir = os.path.join(root, "third_party")
    if os.path.isdir(tp_dir):
        for entry in os.listdir(tp_dir):
            entry_path = os.path.join(tp_dir, entry)
            if not os.path.isdir(entry_path):
                remove_item(entry_path, execute)
                continue
            if entry not in third_party_keep:
                remove_item(entry_path, execute)
            else:
                in_dir = "保留" if not execute else "保留"
                print(f"  [{in_dir}] {entry_path}")
    else:
        print("  third_party/ 目录不存在，跳过")

    # ---- 清理 modules ----
    print("\n--- modules/ 子模块 ---")
    mod_dir = os.path.join(root, "modules")
    if os.path.isdir(mod_dir):
        for entry in os.listdir(mod_dir):
            entry_path = os.path.join(mod_dir, entry)
            if not os.path.isdir(entry_path):
                remove_item(entry_path, execute)
                continue
            if entry not in modules_keep:
                remove_item(entry_path, execute)
            else:
                in_dir = "保留" if not execute else "保留"
                print(f"  [{in_dir}] {entry_path}")
    else:
        print("  modules/ 目录不存在，跳过")

    # ---- 清理 src/ 下未使用的子模块 ----
    remove_specific_subdirs(
        os.path.join(root, "src"),
        src_subdirs_to_delete,
        execute,
        "src/ 下未使用的子模块 (pdf/xps/svg/android)",
    )

    # ---- 清理 include/ 下未使用的子目录 ----
    remove_specific_subdirs(
        os.path.join(root, "include"),
        include_subdirs_to_delete,
        execute,
        "include/ 下未使用的子目录 (android/docs/svg/xps)",
    )

    # ---- 清理 src/gpu/graphite/ 中 CMake 显式排除的子目录 ----
    remove_specific_subdirs(
        os.path.join(root, "src", "gpu", "graphite"),
        graphite_subdirs_to_delete,
        execute,
        "src/gpu/graphite/ 下 CMake 显式排除的子目录 (Android/Vello/dawn/mtl/precompile/sparse_strips/vk)",
    )

    # ---- 删除根目录下的文件 ----
    print("\n--- 根目录下待删除的文件 ---")
    for f in root_files_to_delete:
        path = os.path.join(root, f)
        remove_item(path, execute)

    print(f"\n{'=' * 60}")
    if execute:
        print("  Skia 源码清理完成！")
    else:
        print("  Skia 源码预览结束。")
    print(f"{'=' * 60}\n")


# =============================================================
# 三方库清理 (third/expat, third/harfbuzz, third/icu)
# =============================================================
def cleanup_third_libs(third_dir: str, execute: bool) -> None:
    """三方库源码 (third/expat, third/harfbuzz, third/icu) 的清理。
    CMake 构建只用库源码本身，tests/fuzz/sample/CI/docs/perf 等都可安全清掉。
    """
    action = "执行" if execute else "预览"
    print(f"{'=' * 60}")
    print(f"  三方库目录清理 - {action}模式")
    print(f"  目标: {third_dir}")
    print(f"{'=' * 60}\n")

    # ---- expat ----
    print("--- third/expat/ ---")
    expat_root = os.path.join(third_dir, "expat")
    if os.path.isdir(expat_root):
        # 顶层: testdata 是模糊测试大文件
        remove_item(os.path.join(expat_root, "testdata"), execute)
        # expat 库源码在 expat/expat/lib/，其余子目录全是非构建用
        # 注意: cmake/ 是 expat 自己的 CMakeLists 在 install 时用的, 不能删
        expat_lib_root = os.path.join(expat_root, "expat")
        for d in [
            "conftools",
            "doc",
            "examples",
            "fuzz",
            "gennmtab",
            "tests",
            "win32",
            "xmlwf",
        ]:
            remove_item(os.path.join(expat_lib_root, d), execute)
        # 顶层 Configure / configure.ac / configure / Makefile.* 等 autotools 残留
        for f in [
            "CMakeLists.txt",
            "Configure",
            "configure",
            "configure.ac",
            "configure.bat",
            "Makefile.am",
            "README.md",
            "autogen.sh",
            "m4",
            "test-driver",
        ]:
            remove_item(os.path.join(expat_root, f), execute)
    else:
        print("  third/expat/ 不存在，跳过")

    # ---- harfbuzz ----
    print("\n--- third/harfbuzz/ ---")
    hb_root = os.path.join(third_dir, "harfbuzz")
    if os.path.isdir(hb_root):
        # 非构建用的顶层目录
        for d in [
            ".ci",
            ".circleci",
            ".github",
            "docs",
            "perf",
            "test",
            "util",
        ]:
            remove_item(os.path.join(hb_root, d), execute)
        # 顶层冗余文档/资产
        for f in [
            "BUILD.md",
            "NEWS",
            "README.python.md",
            "RELEASING.md",
            "TESTING.md",
            "meson_options.txt",
            "xkcd.png",
        ]:
            remove_item(os.path.join(hb_root, f), execute)
    else:
        print("  third/harfbuzz/ 不存在，跳过")

    # ---- icu ----
    print("\n--- third/icu/ ---")
    icu_root = os.path.join(third_dir, "icu")
    if os.path.isdir(icu_root):
        icu_src = os.path.join(icu_root, "source")
        if os.path.isdir(icu_src):
            # ICU 构建使用 source/common, source/i18n, source/stubdata, source/data(可选)。
            # 可删除的子目录是 test/tools/samples/python/io/extra/allinone/config。
            # 注意: stubdata 和 data 都是 CMakeLists 引用的, 不能删。
            for d in [
                "allinone",
                "config",
                "extra",
                "io",
                "python",
                "samples",
                "test",
                "tools",
            ]:
                remove_item(os.path.join(icu_src, d), execute)
        else:
            print("  third/icu/source/ 不存在，跳过")
    else:
        print("  third/icu/ 不存在，跳过")

    print(f"\n{'=' * 60}")
    if execute:
        print("  三方库清理完成！")
    else:
        print("  三方库预览结束。")
    print(f"{'=' * 60}\n")


def main():
    args = parse_args()
    root = find_skia_root(args.skia_dir)

    # 默认: 执行模式。仅当显式传 --preview 时才进入预览模式。
    execute = not args.preview

    cleanup_skia(root, execute)

    if not args.skip_third:
        third_dir = resolve_third_dir(root, args.third_dir)
        if third_dir is None:
            print("未找到三方库目录 (默认 <skia>/../third)，跳过三方库清理。")
            print("如需清理，请使用 --third-dir 指定。")
        else:
            cleanup_third_libs(third_dir, execute)
    else:
        print("已通过 --skip-third 跳过三方库清理。")


if __name__ == "__main__":
    main()
