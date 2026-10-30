# Building Kagekidan.exe

The source in this folder is `kagekidan.cc`. It opens a WebView2 window, through the MIT library [webview](https://github.com/webview/webview), onto the page served by `launcher_host.py`.

The packaged program does not look in this folder. Next to `Kagekidan.exe` it expects:

- `python/pythonw.exe`
- `scripts/launcher_host.py`
- `scripts/index.html`
- `scripts/brand/`
- `media/theme.mp3`

## Command

This command compiles the source in this folder. It was run on 30 October 2026 with LLVM MinGW Clang 21.1.3, target `x86_64-w64-windows-gnu`. The same compiler name is embedded in the 29 October test executable. The two files are not byte-identical. The test executable is 1,466,368 bytes. This command produces a larger file because `-g` and `-static` are written down here, not recovered from the old binary.

The Windows libraries it links are the ones the test executable imports: `WS2_32`, `ole32`, `SHELL32`, `SHLWAPI`, `USER32`, `VERSION`, `ADVAPI32`, `KERNEL32`. There is no `libc++.dll`.

```
x86_64-w64-mingw32-clang++ -std=c++17 -O2 -g -static -mwindows \
  -I<webview>/core/include \
  -I<WebView2>/build/native/include \
  kagekidan.cc -o Kagekidan.exe \
  -lws2_32 -lole32 -lshell32 -lshlwapi -luser32 -lversion -luuid -ladvapi32
```

`<webview>/core/include` is the directory that contains `webview/webview.h`, from the webview repository. `<WebView2>/build/native/include` is the native include directory of the Microsoft WebView2 package. It must contain `WebView2.h` and `EventToken.h`. Without `EventToken.h` the build stops.

`Kagekidan.exe` at the repository root is the output of this command. The Windows runtime is rebuilt by [scripts/build_windows.sh](../scripts/build_windows.sh).
