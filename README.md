# Sakura-Wars-PS4RT-Windows

Windows port of [Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), built from tag **v1.1**, commit `29f8822014e8d55d110476088191634f23bc9ce2`. That tag is the code on upstream `main`. The two commits after it on `main` are documents only. The branch `windows` is the default branch.

Published by [DiamondDogz](https://github.com/DiamondDogz). The launcher is `Kagekidan.exe`.

The earlier Windows release, [windows-1.0](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.0), follows upstream v1.0 (`85ac44fdc1fe7e13fac325e6ecea3b523be27d1f`). This branch does not. The game's own version is a separate number: the game must be **01.01**.

Unofficial. Not affiliated with SEGA. The game is not included.

Download: [windows-1.1](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.1). That archive starts the game. In the game, Insert opens the overlay. Graphics preset is there: Max, High, Medium, Lowest.

Tested on an Intel Core i9-12900KF, 128 GB DDR5-4800, an NVIDIA GeForce RTX 3090, and a 4 TB NVMe SSD (PCI Express 4.0). That is the test machine, not a minimum.

Version française : [README.fr.md](README.fr.md).

## What is in this branch

- The v1.1 runtime source, with the Windows changes needed to start it.
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
