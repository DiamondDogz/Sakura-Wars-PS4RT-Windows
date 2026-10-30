# SPDX-License-Identifier: GPL-2.0-or-later
"""Open the visual launcher and start the Windows runtime from it."""
import json
import os
import subprocess
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parent.parent
PAGE = Path(__file__).resolve().parent / "index.html"
BRAND = Path(__file__).resolve().parent / "brand"
STATE = ROOT / "launcher-state.json"
LOG = ROOT / "launcher.log"
PORT_FILE = ROOT / "launcher.port"
MEDIA = {
    "hero.jpg": "image/jpeg",
    "hero.png": "image/png",
    "hero.webp": "image/webp",
    "theme.mp3": "audio/mpeg",
}
OUTPUTS = {1: "1920x1080", 2: "2560x1440", 3: "3840x2160"}
OUTPUT_FLAG = {1: "1080", 2: "1440", 3: "4k"}
LOCK = threading.Lock()
PROCESS = {"handle": None}

DEFAULT = {
    "settings": {
        "outputRes": 1,
        "displayMode": "windowed",
        "upscaler": "off",
        "outlines": 0,
        "framerate": 30,
        "anisotropy": 0,
        "sharpen": False,
        "sharpness": 0,
        "musicVolume": 0.55,
    },
    "paths": {"game": "", "update": "", "dlc": []},
}


def load_state():
    state = json.loads(json.dumps(DEFAULT))
    try:
        saved = json.loads(STATE.read_text(encoding="utf-8"))
        state["settings"].update(saved.get("settings") or {})
        paths = saved.get("paths") or {}
        state["paths"]["game"] = str(paths.get("game") or "")
        state["paths"]["update"] = str(paths.get("update") or "")
        dlc = paths.get("dlc") or []
        state["paths"]["dlc"] = [str(item) for item in dlc if str(item).strip()]
    except (OSError, json.JSONDecodeError, AttributeError):
        pass
    return state


def save_state(state):
    STATE.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")


def media_dir():
    packaged = ROOT / "media"
    names = ("theme.mp3", "hero.jpg", "hero.png", "hero.webp")
    if any((packaged / name).is_file() for name in names):
        return packaged
    return Path(__file__).resolve().parent / "media"


def media_info():
    folder = media_dir()
    hero = next((name for name in ("hero.jpg", "hero.png", "hero.webp") if (folder / name).is_file()), "")
    return {"hero": hero, "track": (folder / "theme.mp3").is_file()}


def ini_text(settings):
    output = OUTPUTS.get(int(settings.get("outputRes") or 1), "1920x1080")
    sharp = float(settings.get("sharpness") or 0)
    sharpen = 1 if settings.get("sharpen") and sharp > 0 else 0
    upscaler = settings.get("upscaler") if settings.get("upscaler") in ("off", "fsr3", "fsr4") else "off"
    mode = settings.get("displayMode") if settings.get("displayMode") in ("windowed", "borderless", "fullscreen") else "windowed"
    lines = [
        "upscaler=" + upscaler,
        "sharpen=" + str(sharpen),
        "sharpness={:.2f}".format(sharp if sharpen else 0),
        "output_res=" + output,
        "display_mode=" + mode,
        "outlines=" + ("2" if int(settings.get("outlines") or 0) == 2 else "0"),
        "framerate=" + ("60" if int(settings.get("framerate") or 30) >= 60 else "30"),
        "anisotropy=" + str(int(settings.get("anisotropy") or 0)),
        "preset=0",
        "rcas=0",
        "fsr1=0",
        "spatial_aa=0",
        "",
    ]
    return "\n".join(lines)


def write_ini(settings):
    text = ini_text(settings)
    (ROOT / "ps4rt.ini").write_text(text, encoding="utf-8")
    target = ROOT / "out" / "CUSA16429"
    target.mkdir(parents=True, exist_ok=True)
    (target / "ps4rt.ini").write_text(text, encoding="utf-8")


def powershell(script):
    completed = subprocess.run(
        ["powershell", "-NoProfile", "-STA", "-Command", script],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    return (completed.stdout or "").strip()


def browse(kind):
    if os.name != "nt":
        raise RuntimeError("picker-unavailable")
    if kind == "update":
        script = (
            "Add-Type -AssemblyName System.Windows.Forms; "
            "$d = New-Object System.Windows.Forms.OpenFileDialog; "
            "$d.Filter = 'PKG (*.pkg)|*.pkg'; "
            "$d.Title = 'Mise a jour 1.01'; "
            "if ($d.ShowDialog() -eq 'OK') { [Console]::Out.Write($d.FileName) }"
        )
        chosen = powershell(script)
        return {"update": chosen} if chosen else {}
    title = "Dossier extrait qui contient eboot.bin" if kind == "game" else "Dossier qui contient les PKG de DLC"
    script = (
        "Add-Type -AssemblyName System.Windows.Forms; "
        "$d = New-Object System.Windows.Forms.FolderBrowserDialog; "
        f"$d.Description = '{title}'; "
        "if ($d.ShowDialog() -eq 'OK') { [Console]::Out.Write($d.SelectedPath) }"
    )
    chosen = powershell(script)
    if not chosen:
        return {}
    if kind == "game":
        return {"game": chosen}
    folder = Path(chosen)
    found = sorted(str(path) for path in folder.rglob("*.pkg") if path.is_file())
    if not found:
        raise RuntimeError("no-pkg")
    return {"dlc": found}


def launch(state):
    game = str(state["paths"].get("game") or "").strip()
    if not game:
        raise RuntimeError("need-game")
    if not Path(game).is_dir():
        raise RuntimeError("missing-game")
    write_ini(state["settings"])
    save_state(state)
    command = [sys.executable, str(ROOT / "scripts" / "ps4rt_launch.py"), "--game", game]
    update = str(state["paths"].get("update") or "").strip()
    if update:
        command += ["--update-pkg", update]
    for pkg in state["paths"].get("dlc") or []:
        if str(pkg).strip():
            command += ["--dlc-pkg", str(pkg)]
    output = int(state["settings"].get("outputRes") or 1)
    upscaler = state["settings"].get("upscaler") or "off"
    if upscaler in ("off", "fsr3", "fsr4"):
        command += ["--upscaler", upscaler]
    if output in OUTPUT_FLAG:
        command += ["--output-resolution", OUTPUT_FLAG[output]]
    flags = getattr(subprocess, "CREATE_NEW_CONSOLE", 0)
    with LOG.open("ab") as log:
        log.write(b"\n--- lancement ---\n")
    PROCESS["handle"] = subprocess.Popen(command, cwd=str(ROOT), creationflags=flags)


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        return

    def send_json(self, code, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def read_json(self):
        length = int(self.headers.get("Content-Length") or 0)
        if length <= 0 or length > 1_000_000:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_GET(self):
        path = urlparse(self.path).path
        if path in ("/", "/index.html"):
            body = PAGE.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)
            return
        if path.startswith("/brand/"):
            name = path.rsplit("/", 1)[-1]
            kind = {"fond-loop.mp4": "video/mp4", "logo.png": "image/png"}.get(name)
            file = BRAND / name
            if kind and file.is_file() and file.resolve().parent == BRAND.resolve():
                body = file.read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", kind)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
        if path.startswith("/media/"):
            name = path.rsplit("/", 1)[-1]
            kind = MEDIA.get(name)
            file = media_dir() / name
            if kind and file.is_file():
                body = file.read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", kind)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
        if path == "/api/state":
            state = load_state()
            state["media"] = media_info()
            self.send_json(200, state)
            return
        if path == "/api/log":
            text = ""
            try:
                text = LOG.read_text(encoding="utf-8", errors="replace")[-4000:]
            except OSError:
                pass
            running = PROCESS["handle"] is not None and PROCESS["handle"].poll() is None
            self.send_json(200, {"running": running, "log": text})
            return
        self.send_error(404)

    def do_POST(self):
        path = urlparse(self.path).path
        try:
            payload = self.read_json()
            with LOCK:
                if path == "/api/state":
                    state = load_state()
                    if isinstance(payload.get("settings"), dict):
                        state["settings"].update(payload["settings"])
                    if isinstance(payload.get("paths"), dict):
                        state["paths"].update(payload["paths"])
                        state["paths"]["dlc"] = [str(item) for item in state["paths"].get("dlc") or [] if str(item).strip()]
                    save_state(state)
                    self.send_json(200, state)
                    return
                if path == "/api/browse":
                    found = browse(str(payload.get("kind") or ""))
                    state = load_state()
                    state["paths"].update(found)
                    save_state(state)
                    self.send_json(200, state["paths"])
                    return
                if path == "/api/play":
                    state = load_state()
                    if isinstance(payload.get("settings"), dict):
                        state["settings"].update(payload["settings"])
                    if isinstance(payload.get("paths"), dict):
                        state["paths"].update(payload["paths"])
                    launch(state)
                    self.send_json(200, {"ok": True})
                    return
        except Exception as error:
            self.send_json(400, {"error": str(error)})
            return
        self.send_error(404)


def serve(open_page=True):
    if PORT_FILE.is_file():
        try:
            existing = int(PORT_FILE.read_text(encoding="utf-8").strip())
            import urllib.request
            with urllib.request.urlopen(f"http://127.0.0.1:{existing}/api/state", timeout=0.4) as response:
                if response.status == 200:
                    if open_page:
                        open_browser(existing)
                    return
        except (OSError, ValueError):
            pass
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    port = server.server_address[1]
    PORT_FILE.write_text(str(port), encoding="utf-8")
    if open_page:
        threading.Timer(0.4, open_browser, args=(port,)).start()
    try:
        server.serve_forever()
    finally:
        try:
            PORT_FILE.unlink()
        except OSError:
            pass


def open_browser(port):
    url = f"http://127.0.0.1:{port}/"
    try:
        if os.name == "nt":
            os.startfile(url)
            return
        webbrowser.open(url)
    except OSError:
        return


if __name__ == "__main__":
    try:
        serve(open_page="--no-browser" not in sys.argv)
    except Exception as error:
        (ROOT / "launcher-error.txt").write_text(str(error), encoding="utf-8")
        raise
