# Install

English first. La version française suit.

## What you need

- Windows 10 or Windows 11, 64-bit.
- A GPU with a current Vulkan driver. NVIDIA or AMD.
- Microsoft Edge WebView2. It is already installed with Edge on current Windows 10 and 11. The launcher does not bundle it.
- Your own copy of Sakura Wars, PlayStation 4, title id CUSA16429, already extracted. The folder must contain `eboot.bin`, `sce_sys` and `sce_module`.
- If that folder is still game version 01.00, your own 01.01 update PKG.
- Optional: your DLC PKG files, all in one folder.

This project does not provide the game, keys, firmware, or saves.

## Unpack

1. Unzip the archive. The folder can keep the name it already has.
2. Do not run the program from inside the zip.
3. `Kagekidan.exe` is at the root of that folder. Open that file.
4. Windows SmartScreen may warn you. The executable is not signed. Choose More info, then Run anyway. Do not turn off Microsoft Defender for this.

The window is the launcher. It does not open your browser.

## Paths

In Paths:

1. Game folder. The extracted CUSA16429 directory, not a PKG.
2. Update. One `.pkg` file, the 01.01 update, only if the folder is still 01.00.
3. DLC. One folder. Every `.pkg` in that folder is taken in one pass. You do not pick them one by one.

The update rewrites files inside the game folder. Copy the folder first if you want to keep 01.00. Saves and the original PKG are not moved.

## Play

Play writes `ps4rt.ini` next to the runtime and starts `ps4rt.exe`. The theme stops. A console window keeps the log.

In game, the overlay is Insert, or L2+R3.

Settings this build actually exposes:

- Output: 1920×1080, 2560×1440, or 3840×2160.
- Windowed, borderless, or fullscreen.
- Original image, FSR 3.1, or FSR 4. FSR 4 falls back to FSR 3.1 if the card cannot run it.
- Native outlines or outlines off.
- 30 or 60 frames per second.
- Anisotropic filtering, applied on the next start.
- Sharpness.

This build includes the v1.1 graphics presets. In game, Insert opens the overlay. Graphics preset is Max, High, Medium, or Lowest. Max is the game's own effects.

## Picture and music

The moving background and the wordmark ship with the launcher. You do not add an official picture.

`media/theme.mp3` is the default theme. Replace that file if you want your own. Keep the name `theme.mp3`.

## Language

English is the default. The EN button switches the launcher to French. The choice is remembered.

## If it fails

- "WebView2" missing: install the Evergreen WebView2 Runtime from Microsoft, then start `Kagekidan.exe` again.
- The launcher cannot see `ps4rt.exe`: the zip was not unpacked in full. `bin/ps4rt.exe` must be present.
- Update refused: the file must be a real `.pkg`, and the game folder must be the extracted CUSA directory, not a path you typed by hand with a missing character.
- Permission denied on the game folder: the folder is read-only or held by another program. Close it, and do not run the launcher from a protected directory without the right to write there.
- A previous update stopped halfway: select the same 01.01 PKG again. The launcher resumes in place.

---

# Installation

## Il vous faut

- Windows 10 ou Windows 11, 64 bits.
- Une carte graphique avec un pilote Vulkan récent. NVIDIA ou AMD.
- Microsoft Edge WebView2. Il est déjà là avec Edge sur les Windows 10 et 11 actuels. Le lanceur ne l’embarque pas.
- Votre propre copie de Sakura Wars, PlayStation 4, identifiant CUSA16429, déjà extraite. Le dossier doit contenir `eboot.bin`, `sce_sys` et `sce_module`.
- Si ce dossier est encore en version 01.00, votre propre PKG de mise à jour 01.01.
- Facultatif : vos PKG de DLC, tous dans un seul dossier.

Ce projet ne fournit ni le jeu, ni les clés, ni le firmware, ni les sauvegardes.

## Décompresser

1. Décompressez l’archive. Le dossier peut garder le nom qu’il a déjà.
2. Ne lancez pas le programme depuis l’intérieur du zip.
3. `Kagekidan.exe` est à la racine. Ouvrez ce fichier.
4. SmartScreen peut prévenir. L’exécutable n’est pas signé. Choisissez Informations complémentaires, puis Exécuter quand même. Ne désactivez pas Microsoft Defender pour cela.

La fenêtre est le lanceur. Elle n’ouvre pas le navigateur.

## Chemins

Dans Chemins :

1. Dossier du jeu. Le dossier CUSA16429 extrait, pas un PKG.
2. Mise à jour. Un seul fichier `.pkg`, la 01.01, seulement si le dossier est encore en 01.00.
3. DLC. Un dossier. Chaque `.pkg` qu’il contient est pris en une fois. On ne les choisit pas un par un.

La mise à jour réécrit des fichiers dans le dossier du jeu. Copiez-le d’abord si vous voulez garder la 01.00. Les sauvegardes et le PKG d’origine ne sont pas déplacés.

## Jouer

Jouer écrit `ps4rt.ini` à côté du runtime et démarre `ps4rt.exe`. Le thème s’arrête. Une fenêtre de console garde le journal.

En jeu, l’incrustation est Insert, ou L2+R3.

Réglages que ce build expose vraiment :

- Sortie : 1920×1080, 2560×1440 ou 3840×2160.
- Fenêtré, sans bordure, ou plein écran.
- Image d’origine, FSR 3.1 ou FSR 4. FSR 4 retombe sur FSR 3.1 si la carte ne le porte pas.
- Contours natifs ou coupés.
- 30 ou 60 images par seconde.
- Filtrage anisotrope, appliqué au démarrage suivant.
- Netteté.

Ce build a les préréglages graphiques de la v1.1. En jeu, Insert ouvre l’incrustation. Le préréglage est Max, High, Medium ou Lowest. Max, ce sont les effets du jeu.

## Image et musique

Le fond animé et le mot partent avec le lanceur. On n’ajoute pas d’illustration officielle.

`media/theme.mp3` est le thème par défaut. Remplacez ce fichier pour le vôtre. Gardez le nom `theme.mp3`.

## Langue

L’anglais est la langue de départ. Le bouton EN passe le lanceur en français. Le choix est retenu.

## Si ça échoue

- WebView2 absent : installez le runtime Evergreen WebView2 chez Microsoft, puis relancez `Kagekidan.exe`.
- Le lanceur ne voit pas `ps4rt.exe` : l’archive n’a pas été décompressée en entier. `bin/ps4rt.exe` doit être là.
- Mise à jour refusée : le fichier doit être un vrai `.pkg`, et le dossier du jeu doit être le dossier CUSA extrait.
- Permission refusée sur le dossier du jeu : il est en lecture seule, ou un autre programme le tient.
- Une mise à jour s’est arrêtée au milieu : resélectionnez le même PKG 01.01. Le lanceur reprend sur place.
