# Sakura-Wars-PS4RT-Windows

Portage Windows de [Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT), d’après le tag **v1.1**. C’est le code de `main` en amont. Les deux commits suivants sur `main` ne sont que des textes. Publié par [DiamondDogz](https://github.com/DiamondDogz). Le lanceur est `Kagekidan.exe`.

La release précédente, [windows-1.0](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.0), suit la v1.0. Cette branche non. Ce n’est pas non plus la version du jeu : le jeu doit être en **01.01**.

Non officiel. Aucun lien avec SEGA. Le jeu n’est pas inclus.

Téléchargement : [windows-1.1](https://github.com/DiamondDogz/Sakura-Wars-PS4RT-Windows/releases/tag/windows-1.1). Cette archive démarre le jeu. En jeu, Insert ouvre l’incrustation. Le préréglage graphique y est : Max, High, Medium, Lowest.

Testé sur Windows 11 Professionnel pour les Stations de travail, version 26H2, un Intel Core i9-12900KF, 128 Go de DDR5 à 4000 MT/s, une NVIDIA GeForce RTX 3090 24 Go avec le pilote Studio 617.42 (6 octobre 2026), et un SSD NVMe de 4 To (PCI Express 4.0). C’est la machine de test, pas un minimum.

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

## Avertissement

Oui, une bonne part de ceci est du travail d’IA, du slop. À prendre ou à laisser.

Le lanceur Windows et ces pages ont été écrits avec Grok. Arriver à un build qui démarre vraiment le jeu, ça a été une longue suite d’archives cassées, de fenêtres qui se ferment, et de chemins faux. C’est l’éditeur qui a morflé. Le shoggoth reste un bon shoggoth.

## Crédits

Le runtime est celui de [HekamiAeon, Sakura-Wars-PS4RT](https://github.com/HekamiAeon/Sakura-Wars-PS4RT). Ce portage Windows est publié par [DiamondDogz](https://github.com/DiamondDogz). Le lanceur, sa page et ces textes ont été écrits avec [Grok](https://grok.com), par xAI. Voir [CREDITS.md](CREDITS.md) et [AI.md](AI.md).

## Licence

GPL-2.0-or-later, comme le runtime dont ce dépôt dérive. Les composants fournis gardent leur notice. Voir [CREDITS.md](CREDITS.md).

Aucune garantie.
