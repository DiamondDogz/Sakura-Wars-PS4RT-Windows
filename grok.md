# Mémoire du projet

Lire ce fichier avant de modifier le dépôt, le lanceur ou le runtime. Il résume toute la conversation, du premier message jusqu'au 30 octobre 2026. Ne pas le contredire. Ne pas le raccourcir en « premier dépôt » ou en tutoriel à faire cliquer.

La personne parle français, va droit au but, et veut que le travail soit fait. Répondre en français, court. Ne pas lui donner une procédure à exécuter quand on peut le faire soi-même. Ne pas lui demander de cliquer sur Revert, de coller un chemin, ou de réparer un fichier à la main.

## Quoi

Portage Windows de [HekamiAeon/Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), d’après le tag **v1.1**, commit `29f8822014e8d55d110476088191634f23bc9ce2`. C’est le code de `main` en amont. Les deux commits après ce tag ne sont que des textes. La branche par défaut est `windows`, avancée sur cette ligne le 31 octobre 2026 après un test réussi de l’archive `windows-1.1-test`.

La release stable à donner est `windows-1.1.2`. `windows-1.1.3` est expérimentale : trophées lus en local, carte dans le jeu. Qui n’en veut pas reste sur `windows-1.1.2`. `windows-1.0` reste en ligne : elle suit la v1.0, commit `85ac44fdc1fe7e13fac325e6ecea3b523be27d1f`. Ne pas la supprimer. Ne pas revenir à la v1.0. Ne pas remettre le bouton qui débloque un trophée à la main.

Machine de test, pas un minimum : Windows 11 Professionnel pour les Stations de travail, version 26H2. Intel Core i9-12900KF, 128 Go DDR5 à 4000 MT/s (Windows l’affiche ainsi ; ne pas écrire 4800 ni 48000). NVIDIA GeForce RTX 3090 24 Go, pilote Studio 617.42 du 6 octobre 2026. SSD NVMe 4 To Gen 4. Ne pas publier le nom du PC, l’ID de périphérique, ni l’ID produit.

Le jeu lui-même doit être en **01.01**. Ce numéro n'est pas celui du runtime.

Dépôt : `DiamondDogz/Sakura-Wars-PS4RT-Windows`. Compte qui commit : `shinseiryuu`. Branche par défaut : `windows`. Dernière tête connue avant ce fichier : `5b63ed80c8c858c2cc1ba11364cd438fbbc13319`.

C'est un fork du runtime, plus un lanceur Windows écrit par-dessus. Le runtime n'est pas une réécriture. Le crédit du runtime reste à HekamiAeon et aux personnes de `CREDITS.md`. Le lanceur, sa page et les textes ont été écrits avec Grok, par xAI. Le dire dans les crédits. Ne jamais écrire que c'est un premier dépôt.

Non officiel. Aucun lien avec SEGA. Le jeu n'est pas dans le dépôt.

## À quoi ça sert

Lancer sa propre copie déjà extraite de Sakura Wars, PlayStation 4, CUSA16429, depuis Windows, sans ouvrir un navigateur.

Le programme que l'on double-clique s'appelle `Kagekidan.exe`. Il est à la racine du dossier, pas dans un sous-dossier `launcher`, et ce n'est pas une page HTML. Il ouvre une fenêtre WebView2. Derrière : `launcher/kagekidan.cc`, `launcher_host.py`, `index.html`.

À côté de l'exe, le paquet attend :

- `python/pythonw.exe`
- `scripts/launcher_host.py`
- `scripts/index.html`
- `scripts/brand/`
- `media/theme.mp3`
- `bin/ps4rt.exe`

`Jouer` écrit `ps4rt.ini` et démarre le runtime. Le thème musical s'arrête à ce moment-là. Une console garde le journal.

## Machine déjà en place


- Dossier du jeu déjà extrait, passé en 01.01 : `C:\RetroBat\roms\ps4\st\EP0177-CUSA16429_00-NEWSAKURAWARSAPP`
- Neuf DLC installés dans ce dossier. Ne pas les réinstaller pour un test.

Le dossier du jeu contient `eboot.bin`, pas un PKG. Le PKG est l'installateur. L'extraction a déjà été faite. Une mise à jour 01.01 a déjà été appliquée sur place. Un essai s'est arrêté sur un refus d'écriture, puis a repris et a fini. Code de fin 0.

PowerShell refusait les scripts (« l'exécution de scripts est désactivée »). WSL a été abandonné. Il faut un vrai programme Windows, pas un script à autoriser.

## Décisions figées

- Un seul projet, corrigé sur place. Pas une nouvelle archive à chaque correction.
- Le lanceur est un exe à la racine. Nom de l'exe : `Kagekidan.exe` seulement. Ne pas renommer le dépôt sans redemander le nom exact. Le nom public actuel reste `Sakura-Wars-PS4RT-Windows`.
- Anglais par défaut. Le bouton EN passe en français. Le choix est retenu.
- Interface verre, fond animé, textes qui suivent la taille de l'image. Pas de mentions d'écran en trop.
- `media/theme.mp3` démarre tout seul, pas en sourdine, et s'arrête quand le jeu part.
- En production, l'interface n'édite pas le dossier media. On remplace les fichiers à la main. Image et `theme.mp3`, mêmes noms.
- Les réglages du lanceur restent ceux du build : 1080p, 1440p, 2160p ; fenêtre, sans bordure, plein écran ; image d’origine, FSR 3.1, FSR 4 (repli sur 3.1) ; contours natifs ou coupés ; 30 ou 60 images ; anisotrope au prochain démarrage ; netteté. Les préréglages v1.1 Max, High, Medium, Lowest sont dans l’incrustation en jeu (Insert), pas dans le lanceur. Max, ce sont les effets du jeu.
- Chemins dans l'interface : dossier du jeu, un PKG de mise à jour, un dossier de DLC. Tous les `.pkg` de ce dossier partent ensemble. Pas un bouton « ajouter un DLC » un par un. Pas de chemin à coller : un explorateur.
- DLSS reste éteint. Les sources du pont DLSS ne sont pas dans l'arbre. FSR reste.
- Le mot du logo est `Kagekidan` en latin. Le K du monogramme a été remplacé par les kanjis **華撃団**, pas par un autre mot. Ce sont ceux de 帝国華撃団. Ils ont été composés avec Noto Serif JP, pas dessinés par un modèle. Le fond animé et la fleur sont générés, sans illustration officielle.
- Thème : *さくらさくら*, mélodie traditionnelle, domaine public, publiée en 1888. Le fichier est une synthèse à cordes pincées, pas un enregistrement historique, pas l'opening commercial. On peut le remplacer en gardant le nom `theme.mp3`.
- Licence du runtime : GPL-2.0-or-later. Pas de binaire GPL sans la source qui le produit. `ps4rt.exe` et `Kagekidan.exe` sont dans le dépôt, avec la source qui les produit. L’extracteur `bin/ps4rt-pkg.exe` qui ouvre les PKG est l’ancien binaire de 2 332 672 octets. Ne pas le remplacer par une reconstruction statique de Crypto++ : elle corrompt le tas.

## Ce qui a déjà marché chez lui

L'ancien `ps4rt.exe` a : vu le dossier, demandé la 01.01, extrait le PKG de mise à jour, écrit les fichiers, installé les neuf DLC, puis échoué au lancement.

La ligne d'arrêt :

`STOP: reserving the PS4 address range (link with --disable-high-entropy-va) failed at 0000001000000000 (+0xec00000000), error 487`

Erreur 487 : Windows a refusé de réserver la plage mémoire du jeu. La correction côté source est d'appeler `ps4rt_VirtualAlloc2` dans `src/compat_win.c`, plus les adaptations Windows listées plus bas. Relancer l'ancien exe ne teste rien : il réaffichera 487.

Le test, c'est un `ps4rt.exe` recompilé depuis cette branche, mis à la place de l'ancien, puis Jouer. La mise à jour et les DLC ne sont pas à refaire. C'est bon si la fenêtre du jeu s'ouvre et que `error 487` n'apparaît plus. Ce nouvel exe n'existe pas encore dans le dépôt.

## Sources Windows déjà poussées

Vérifiées par taille égale au fichier local :

| Fichier | Correction |
|---|---|
| `src/runtime_memory.c` | appelle `ps4rt_VirtualAlloc2` |
| `src/compat_win.c` | aides Windows, dont `ps4rt_VirtualAlloc2` |
| `src/runtime_file.c` | `mkdir` Windows |
| `src/runtime_savedata.c` | dossiers de sauvegarde Windows |
| `gpu/CMakeLists.txt` | `ntapi.cpp`, `WINVER` et `_WIN32_WINNT` à `0x0A00` |
| `gpu/shim/ps4rtgpu.cpp` | `NtApi::Initialize` avant le GPU |
| `gpu/shadps4/common/io_file.cpp` | `CreateFileMappingW` |
| `gpu/shadps4/common/path_util.cpp` | `shlobj.h` |
| `gpu/shadps4/video_core/renderer_vulkan/vk_dlss.cpp` | DLSS laissé éteint |
| `third_party/ps4pkg/common/io_file.h` | `_wfopen` |
| `third_party/ps4pkg/core/file_format/pkg.cpp` | messages de progression |

`gpu/third_party/fsr-vulkan/upstream/ffx-1.1.4/sdk/src/backends/vk/ffx_vk.cpp` a été écrasé par erreur avec 25 octets, le texte `PLACEHOLDER_WILL_NOT_SEND`, commit `9bbb1b82`. Ne jamais refaire ça. Le fichier fait 239567 octets et l'API refuse cette taille d'un bloc.

Il a été remis en le coupant en sept morceaux, commit `5b63ed80`. `ffx_vk.cpp` ne contient plus que :

```cpp
#include "ffx_vk_1.inc"
#include "ffx_vk_2.inc"
#include "ffx_vk_3.inc"
#include "ffx_vk_4.inc"
#include "ffx_vk_5.inc"
#include "ffx_vk_6.inc"
#include "ffx_vk_7.inc"
```

Collés dans l'ordre, les `.inc` sont exactement les 239567 octets d'origine, y compris `#include <mutex>`. Ne pas les « résumer ». Ne pas recompiler les `.inc` tout seuls : seul `ffx_vk.cpp` est une unité de traduction.

## Compiler le lanceur

Commande notée le 30 octobre 2026, LLVM MinGW Clang 21.1.3, cible `x86_64-w64-windows-gnu`. L'exe du 29 octobre faisait 1466368 octets et n'est pas identique à celui que produit cette ligne, à cause de `-g` et `-static`.

```
x86_64-w64-mingw32-clang++ -std=c++17 -O2 -g -static -mwindows \
  -I<webview>/core/include \
  -I<WebView2>/build/native/include \
  kagekidan.cc -o Kagekidan.exe \
  -lws2_32 -lole32 -lshell32 -lshlwapi -luser32 -lversion -luuid -ladvapi32
```

Il faut `webview/webview.h` et, dans le paquet WebView2, `WebView2.h` plus `EventToken.h`. Sans `EventToken.h` la compilation s'arrête. Pas de `libc++.dll` : lien statique. Détail dans `launcher/BUILD.md`.

La commande qui produit `ps4rt.exe` depuis cette branche n'est pas encore une recette vérifiée de bout en bout. Ne pas publier un exe dont on ne peut pas montrer la commande.

## Pièges déjà payés

- L'API GitHub qui écrit un fichier corrompt les binaires : les octets à partir de `0x80` gonflent en UTF-8. Images, mp3 et mp4 passent par un envoi manuel, jamais par cet appel.
- Un fichier texte d'environ 40 Ko passe. 239 Ko ne passent pas. Si l'envoi ne peut pas contenir tout le fichier, on n'envoie rien. Surtout pas un texte court à la place.
- Après un envoi, la taille renvoyée doit être celle du fichier local. Sinon on s'arrête.
- Le cache `raw.githubusercontent.com` peut encore montrer l'ancien fichier. Vérifier avec le SHA du commit, pas avec la branche en cache.
- Les zip proposés dans le chat sont parfois bloqués. Ne pas compter dessus comme seul moyen de livrer.
- L'appli GitHub doit être installée sur ce dépôt. Un bouton Connect gris ne se règle pas en devinant des droits.

## Documents déjà en place

`README.md`, `README.fr.md`, `INSTALL.md`, `WINDOWS.md`, `CREDITS.md`, `AI.md`, `launcher/BUILD.md`.

Le paragraphe qui décrivait la boucle de pétales dans le README a été retiré. Le crédit Grok reste. `AI.md` dit où l'IA a été utilisée et où elle ne devait pas fournir le jeu, l'illustration officielle, l'opening commercial, une promesse v1.1, ou des kanjis inventés.

## Suite

1. La branche `windows` suit la v1.1. Ne pas la ramener à la v1.0.
2. `windows-1.1.2` est l’archive stable. `windows-1.1.3` est l’essai des trophées. `windows-1.0` reste la précédente.
3. Les préréglages s’appliquent avec `VirtualProtect` dans `gpu/shim/ps4rt_effects.cpp`. Ne pas remettre le `Protect` Windows qui renvoyait faux.
