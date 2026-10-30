# Sakura-Wars-PS4RT-Windows

Windows launcher for [Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), built from tag **v1.0**, commit `85ac44fdc1fe7e13fac325e6ecea3b523be27d1f`. The branch that carries this work is `windows`. The default branch is newer and is not used.

The launcher executable is `Kagekidan.exe`. This file does not replace the upstream README.

This is not the v1.1 runtime. v1.1 was tagged on 30 October 2026 at `29f8822014e8d55d110476088191634f23bc9ce2` and is not the base of this branch. The game's own version is a separate number: the game must be **01.01**.

Unofficial. Not affiliated with SEGA. The game is not included.

## What is in this branch

- The v1.0 runtime source, unchanged, from the upstream tag.
- `launcher/`: the source of `Kagekidan.exe`, its page, and the default theme.
- [INSTALL.md](INSTALL.md), [CREDITS.md](CREDITS.md), [AI.md](AI.md), and [README.fr.md](README.fr.md).

`ps4rt.exe` is not in this commit. A binary of the runtime is not published until it can be rebuilt from the source in this branch.

## Install

See [INSTALL.md](INSTALL.md). Short form: Windows 10 or 11, your own extracted CUSA16429 folder, your own 01.01 update PKG if that folder is still 01.00, then `Kagekidan.exe`.

## Launcher

The petal loop and the wordmark ship with the launcher. **華撃団** is set where the monogram K used to be. The default theme is this project's performance of *さくらさくら*, a traditional koto melody published in 1888. It is public domain. The file is synthesized. It is not a historical recording. Replace `launcher/media/theme.mp3` if you want another track. The theme stops when the game starts.

English is the launcher language. The EN button switches to French.

`Kagekidan.exe` is not signed. SmartScreen may warn. Do not turn off Microsoft Defender.

## License

GPL-2.0-or-later, the same terms as the runtime. Third-party pieces keep their own notices. See [CREDITS.md](CREDITS.md).

There is no warranty.
