@echo off
cd /d "%~dp0"
if not exist "%~dp0Kagekidan.exe" (
  echo Kagekidan.exe introuvable a cote de ce fichier.
  pause
  exit /b 1
)
start "" "%~dp0Kagekidan.exe"
exit /b 0
