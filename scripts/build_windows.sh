#!/usr/bin/env bash
# Rebuild the Windows programs from this checkout.
# Requires LLVM MinGW Clang 21 (x86_64-w64-mingw32-clang), a mingw64 prefix
# with SDL3, FFmpeg, Vulkan, xxhash, Zydis and Boost, plus a host glslangValidator.
#   MINGW_ROOT   LLVM MinGW prefix
#   MSYS_PREFIX  mingw64 prefix
#   GLSLANG      host glslangValidator
#   WEBVIEW_INCLUDE and WV2_INCLUDE for Kagekidan.exe
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
: "${MINGW_ROOT:?Set MINGW_ROOT}"
: "${MSYS_PREFIX:?Set MSYS_PREFIX}"
: "${GLSLANG:?Set GLSLANG}"
export PATH="$MINGW_ROOT/bin:$PATH"
CC=x86_64-w64-mingw32-clang
CXX=x86_64-w64-mingw32-clang++

cmake -S gpu -B out/gpu-windows -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/scripts/mingw-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS4RT_PGO=off -DPS4RT_LTO=OFF \
  -DPKG_CONFIG_EXECUTABLE="$PWD/scripts/mingw-pkg-config" \
  -DGLSLANG_COMPILER="$GLSLANG"
cmake --build out/gpu-windows --target ps4rtgpu -j "${PS4RT_BUILD_JOBS:-$(nproc)}"

obj=out/ps4rt-windows
rm -rf "$obj"
mkdir -p "$obj/atrac9"
inc=(-I. -Isrc -I"$MSYS_PREFIX/include")
for source in third_party/LibAtrac9/C/src/*.c; do
  "$CC" -std=c99 -O2 -w -c "$source" -o "$obj/atrac9/$(basename "${source%.c}").o"
done
"$CC" -std=c11 -O2 -Wall -Wextra -Werror -mgeneral-regs-only -c src/isa_emulate.c -o "$obj/isa_emulate.o"
"$CC" -c src/compat_win_jmp.s -o "$obj/compat_win_jmp.o"
for source in src/compat_win.c src/loader.c src/cpu_check.c src/isa_rewrite.c src/runtime.c src/runtime_*.c; do
  "$CC" -std=c11 -O2 -Wall -Wextra -Werror -DPS4RT_RUNTIME -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 \
    "${inc[@]}" -c "$source" -o "$obj/$(basename "${source%.c}").o"
done
"$CXX" -std=c11 -O2 -o bin/ps4rt.exe \
  "$obj"/*.o "$obj"/atrac9/*.o \
  -Lout/gpu-windows -lps4rtgpu \
  -Lout/gpu-windows/third_party/fsr-vulkan -lffx_vulkan_portable -lffx_vulkan_fsr3_vk_backend_1_1_4 \
    -lffx_vulkan_fsr3_host_1_1_4 -lffx_vulkan_fsr4_v07_vulkan -lffx_vulkan_fsr4_v07_assets \
  -Lout/gpu-windows/third_party/sirit/src -lsirit \
  -Lout/gpu-windows/third_party/miniz -lminiz \
  -L"$MSYS_PREFIX/lib" \
  -lvulkan-1 -lSDL3 -lavformat -lavcodec -lavutil -lswscale -lswresample -lxxhash -lZydis \
  -lbcrypt -lws2_32 -lshell32 -luser32 -lwinpthread -lpdh \
  -Wl,--disable-high-entropy-va

if [[ -n ${WEBVIEW_INCLUDE:-} && -n ${WV2_INCLUDE:-} ]]; then
  "$CXX" -std=c++17 -O2 -g -static -mwindows \
    -I"$WEBVIEW_INCLUDE" -I"$WV2_INCLUDE" \
    launcher/kagekidan.cc -o Kagekidan.exe \
    -lws2_32 -lole32 -lshell32 -lshlwapi -luser32 -lversion -luuid -ladvapi32
fi
