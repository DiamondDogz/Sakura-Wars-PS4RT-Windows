# Building Kagekidan.exe

The source in this folder is `kagekidan.cc`. It opens a WebView2 window, through the MIT library [webview](https://github.com/webview/webview), onto the page served by `launcher_host.py`.

The packaged program does not look in this folder. Next to `Kagekidan.exe` it expects:

- `python/pythonw.exe`
- `scripts/launcher_host.py`
- `scripts/index.html`
- `scripts/brand/`
- `media/theme.mp3`

`Kagekidan.exe` is not in this commit. The exact compile command used for the test build was not kept, so it is not written here as if it were reproducible.
