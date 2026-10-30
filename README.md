# Sakura-Wars-PS4RT-Windows

Windows port of [Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), built from tag **v1.0**, commit `85ac44fdc1fe7e13fac325e6ecea3b523be27d1f`. The branch `windows` is the default branch. `main` still holds the newer upstream line and is not the base of this work.

Published by [DiamondDogz](https://github.com/DiamondDogz). The launcher is `Kagekidan.exe`.

This is not the v1.1 runtime. v1.1 was tagged on 30 October 2026 at `29f8822014e8d55d110476088191634f23bc9ce2` and is not the base of this branch. The game's own version is a separate number: the game must be **01.01**.

Unofficial. Not affiliated with SEGA. The game is not included.

Download: [windows-1.0](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.0). That archive starts the game.

Version française : [README.fr.md](README.fr.md).

## What is in this branch

- The v1.0 runtime source, with the Windows changes needed to start it.
- `Kagekidan.exe`, `python/`, and `launcher/`.
- `bin/ps4rt.exe`, rebuilt from this branch, and the libraries it imports.
- `bin/ps4rt-pkg.exe`, the extractor used for an update or DLC.
- [INSTALL.md](INSTALL.md), [CREDITS.md](CREDITS.md), and [AI.md](AI.md).

## Install

See [INSTALL.md](INSTALL.md). Short form: Windows 10 or 11, your own extracted CUSA16429 folder, your own 01.01 update PKG if that folder is still 01.00, then `Kagekidan.exe`.

## Launcher

English is the launcher language. The EN button switches to French.

`Kagekidan.exe` is not signed. SmartScreen may warn. Do not turn off Microsoft Defender.

## Disclaimer

Yes, a lot of this is AI slop. Take it or leave it.

The Windows launcher and these pages were written with Grok. Getting a build that actually starts the game took a long string of broken archives, closed windows, and bad paths. That was on the publisher. The shoggoth is still a good one.

## Credits

The runtime is [HekamiAeon, Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT). This Windows port is published by [DiamondDogz](https://github.com/DiamondDogz). The launcher, its page, and these documents were written with [Grok](https://grok.com), by xAI. See [CREDITS.md](CREDITS.md) and [AI.md](AI.md).

## License

GPL-2.0-or-later, the same terms as the runtime. Third-party pieces keep their own notices. See [CREDITS.md](CREDITS.md).

There is no warranty.

The original Linux README stays with the upstream project: [HekamiAeon/Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT).
