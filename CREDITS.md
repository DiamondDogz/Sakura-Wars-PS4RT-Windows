# Credits and legal notes

This is an unofficial Windows port. It does not replace, and is not endorsed by, SEGA. The game is *新サクラ大戦*, Sakura Wars, published by SEGA. The name is used to say which title this program runs.

## Runtime

- [HekamiAeon, Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT). The runtime. GPL-2.0-or-later. Written for Linux. This port follows the v1.1 release of 30 October 2026, commit `29f8822014e8d55d110476088191634f23bc9ce2`, on the branch `windows`. Upstream `main` is that commit plus two documentation commits. The earlier Windows release follows v1.0, commit `85ac44fdc1fe7e13fac325e6ecea3b523be27d1f`. This is not a general PS4 emulator.
- [Felipe Prado, gow3_pc](https://github.com/FelipePrado0/gow3_pc). The compatibility runtime this line of work comes from.
- [deadinside28](https://github.com/deadinside28/bloodborne_pc) and [Supermedo](https://github.com/Supermedo/bloodborne_pc), bloodborne_pc.
- [shadPS4](https://github.com/shadps4-emu/shadPS4) and its contributors, including cuesta4, for the Vulkan renderer and the shader recompiler.
- [AzaharPlus, shadPS4Plus](https://github.com/AzaharPlus/shadPS4Plus). PKG extraction. GPL-2.0-or-later.

## Windows port

- Published by [DiamondDogz](https://github.com/DiamondDogz). This is the Windows packaging and the launcher around the v1.1 runtime. It is not a claim on the runtime, and not a claim on the game.
- [Grok](https://grok.com), by xAI. The Windows launcher, its page, and the documents in this branch were written with Grok. The runtime was not rewritten by it. See [AI.md](AI.md).

## Libraries in the Windows build

- [AMD FidelityFX Super Resolution](https://gpuopen.com/fidelityfx-super-resolution/). The FSR4 v07 model assets shipped beside the runtime carry their own notice, `fsr4_shaders/LICENSE-FSR4-v07.txt`. That notice identifies them as MIT, copyright 2025 Advanced Micro Devices, Inc., taken from revision `01446e6a74888bf349652fcf2cbf5f642d30c2bf` of `WinLinux1028/FidelityFX-SDK_WithFSR4`, through the Q2RTX shader build. The notice stays with the files.
- SDL3. Its license stays with the binary.
- FFmpeg and the codecs linked into this build. `libx264` and `libx265` are GPL-2. Their presence means this build is not an LGPL-only FFmpeg. That is compatible with GPL-2.0-or-later. It is not compatible with a claim that the media libraries are LGPL only.
- Crypto++, zlib, Zydis, ImGui. Each keeps its notice.
- Python, embedded to run the launcher host. The PSF license is `python/LICENSE.txt` in the archive.
- [webview](https://github.com/webview/webview), used by `Kagekidan.exe`. MIT.
- Microsoft Edge WebView2. Not bundled. The user installs or already has the Evergreen runtime.
- Noto Serif JP, SIL Open Font License. Used to draw the characters 華撃団 so the strokes are the font's. The font file is not shipped. The picture of those characters is.

## Words and music

- 華撃団 is the name of the troupe in Sakura Taisen. The full name in the work is 帝国華撃団. The three characters on the launcher are that name, set in type. They are not the official logo.
- *さくらさくら*. Traditional koto melody, published in the Ministry collection *箏曲集* in 1888. Author unknown. The composition is in the public domain. The file in `media/theme.mp3` is a new synthesized performance made for this project. It is not a period recording, and it is not Kimio Eto's recording, which is not public domain.

## What this repository must not contain

- The game, any PKG, keys, firmware, or saves.
- Official key art, the opening theme, or any other SEGA recording.
- A binary of `ps4rt.exe` published without the corresponding source of the Windows changes.

## Release

The release [windows-1.1](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.1) is the player archive for this branch. [windows-1.0](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.0) is the previous archive, built from upstream v1.0. The notes give the SHA-256. The current archive starts CUSA16429 at game version 01.01. `Kagekidan.exe` is unsigned. Do not tell people to disable their antivirus.
