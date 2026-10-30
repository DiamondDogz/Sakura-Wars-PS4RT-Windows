# Sakura-Wars-PS4RT-Windows

Build Windows de [Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), d’après le tag **v1.0**. Le lanceur est `Kagekidan.exe`.

Ce n’est pas le runtime v1.1. La v1.1 est sortie le 30 octobre 2026 et n’est pas incluse. Ce n’est pas non plus la version du jeu : le jeu doit être en **01.01**.

Non officiel. Aucun lien avec SEGA. Le jeu n’est pas inclus.

## État

Les programmes sont dans cette branche : `Kagekidan.exe`, `bin/ps4rt.exe`, `bin/ps4rt-pkg.exe`. La release `windows-1.0` porte l’archive et son empreinte SHA-256. Les sources de ce portage sont le tag correspondant.

## Installation

Le détail est dans [INSTALL.md](INSTALL.md). En bref :

1. Windows 10 ou 11, x86-64, avec Microsoft Edge WebView2.
2. Un pilote Vulkan pour la carte graphique.
3. Votre propre dossier déjà extrait de Sakura Wars, CUSA16429.
4. Votre propre PKG de mise à jour 01.01, si ce dossier est encore en 01.00.
5. Décompressez l’archive. Ouvrez `Kagekidan.exe`, à la racine. Il n’ouvre pas le navigateur.
6. Indiquez le dossier du jeu, le PKG de mise à jour, puis le dossier des DLC. Jouer écrit `ps4rt.ini` et démarre le runtime.

## Lanceur

L’anglais est la langue du lanceur. Le bouton EN passe au français.

## Crédits

Le runtime est celui de [HekamiAeon, Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT). Le lanceur Windows, sa page et ces textes ont été écrits avec [Grok](https://grok.com), par xAI. Voir [CREDITS.md](CREDITS.md) et [AI.md](AI.md).

## Licence

GPL-2.0-or-later, comme le runtime dont ce dépôt dérive. Les composants fournis gardent leur notice. Voir [CREDITS.md](CREDITS.md).

Aucune garantie.
