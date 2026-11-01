"""Read a local PS4 trophy00.trp. Nothing in this module is sent anywhere."""

from __future__ import annotations

import json
import struct
import xml.etree.ElementTree as ET
from pathlib import Path

# Public NP trophy keygen key. The communication id is read from the file.
_TROPHY_KEY = bytes((
    0x21, 0xF4, 0x1A, 0x6B, 0xAD, 0x8A, 0x1D, 0x3E,
    0xCA, 0x7A, 0xD5, 0x86, 0xC1, 0x01, 0xB7, 0xA9,
))
_SBOX = (
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
)
_RCON = (0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36)
_LANGUAGES = (
    ("TROP.ESFM", "en", "English"),
    ("TROP_00.ESFM", "ja", "日本語"),
    ("TROP_02.ESFM", "fr", "Français"),
    ("TROP_03.ESFM", "es", "Español"),
    ("TROP_04.ESFM", "de", "Deutsch"),
    ("TROP_09.ESFM", "ko", "한국어"),
    ("TROP_10.ESFM", "zh-Hant", "繁體中文"),
)
_GRADES = {"P": "Platinum", "G": "Gold", "S": "Silver", "B": "Bronze"}


def _xtime(value: int) -> int:
    return ((value << 1) ^ 0x1B) & 0xFF if value & 0x80 else (value << 1) & 0xFF


def _words(key: bytes) -> list[list[int]]:
    words = [list(key[index:index + 4]) for index in range(0, 16, 4)]
    for index in range(4, 44):
        word = words[index - 1][:]
        if index % 4 == 0:
            word = word[1:] + word[:1]
            word = [_SBOX[byte] for byte in word]
            word[0] ^= _RCON[index // 4]
        words.append([left ^ right for left, right in zip(words[index - 4], word)])
    return words


def _add(state: list[int], words: list[list[int]], start: int) -> None:
    for column in range(4):
        for row in range(4):
            state[row + column * 4] ^= words[start + column][row]


def _encrypt_block(block: bytes, words: list[list[int]]) -> bytes:
    state = list(block)
    _add(state, words, 0)
    for round_index in range(1, 10):
        state = [_SBOX[byte] for byte in state]
        for row in range(1, 4):
            cells = [state[row + column * 4] for column in range(4)]
            cells = cells[row:] + cells[:row]
            for column in range(4):
                state[row + column * 4] = cells[column]
        mixed = state[:]
        for column in range(4):
            a, b, c, d = (state[row + column * 4] for row in range(4))
            mixed[column * 4] = _xtime(a) ^ _xtime(b) ^ b ^ c ^ d
            mixed[1 + column * 4] = a ^ _xtime(b) ^ _xtime(c) ^ c ^ d
            mixed[2 + column * 4] = a ^ b ^ _xtime(c) ^ _xtime(d) ^ d
            mixed[3 + column * 4] = _xtime(a) ^ a ^ b ^ c ^ _xtime(d)
        state = mixed
        _add(state, words, round_index * 4)
    state = [_SBOX[byte] for byte in state]
    for row in range(1, 4):
        cells = [state[row + column * 4] for column in range(4)]
        cells = cells[row:] + cells[:row]
        for column in range(4):
            state[row + column * 4] = cells[column]
    _add(state, words, 40)
    return bytes(state)


def _decrypt_block(block: bytes, words: list[list[int]]) -> bytes:
    inverse = [0] * 256
    for index, value in enumerate(_SBOX):
        inverse[value] = index

    def mul(value: int, times: int) -> int:
        out = 0
        while times:
            if times & 1:
                out ^= value
            value = _xtime(value)
            times >>= 1
        return out

    state = list(block)
    _add(state, words, 40)
    for round_index in range(9, 0, -1):
        for row in range(1, 4):
            cells = [state[row + column * 4] for column in range(4)]
            cells = cells[-row:] + cells[:-row]
            for column in range(4):
                state[row + column * 4] = cells[column]
        state = [inverse[byte] for byte in state]
        _add(state, words, round_index * 4)
        mixed = state[:]
        for column in range(4):
            a, b, c, d = (state[row + column * 4] for row in range(4))
            mixed[column * 4] = mul(a, 14) ^ mul(b, 11) ^ mul(c, 13) ^ mul(d, 9)
            mixed[1 + column * 4] = mul(a, 9) ^ mul(b, 14) ^ mul(c, 11) ^ mul(d, 13)
            mixed[2 + column * 4] = mul(a, 13) ^ mul(b, 9) ^ mul(c, 14) ^ mul(d, 11)
            mixed[3 + column * 4] = mul(a, 11) ^ mul(b, 13) ^ mul(c, 9) ^ mul(d, 14)
        state = mixed
    for row in range(1, 4):
        cells = [state[row + column * 4] for column in range(4)]
        cells = cells[-row:] + cells[:-row]
        for column in range(4):
            state[row + column * 4] = cells[column]
    state = [inverse[byte] for byte in state]
    _add(state, words, 0)
    return bytes(state)


def _communication_key(sample: bytes) -> list[list[int]]:
    master = _words(_TROPHY_KEY)
    order = (17587, *range(17587), *range(17588, 100000))
    for number in order:
        identity = f"NPWR{number:05d}_00".encode().ljust(16, b"\0")
        derived = _encrypt_block(identity, master)
        words = _words(derived)
        plain = bytes(left ^ right for left, right in zip(_decrypt_block(sample[16:32], words), sample[:16]))
        if plain.startswith(b"<!--") or plain.startswith(b"<?xml") or plain.startswith(b"<trophy"):
            return words
    raise ValueError("trophy communication id was not found")


def _decrypt_esfm(blob: bytes, words: list[list[int]]) -> str:
    iv, data = blob[:16], blob[16:]
    data = data[: len(data) // 16 * 16]
    out = bytearray()
    previous = iv
    for offset in range(0, len(data), 16):
        plain = bytes(left ^ right for left, right in zip(_decrypt_block(data[offset:offset + 16], words), previous))
        previous = data[offset:offset + 16]
        out.extend(plain)
    pad = out[-1]
    if 1 <= pad <= 16 and out.endswith(bytes([pad]) * pad):
        del out[-pad:]
    text = out.decode("utf-8")
    start = text.find("<trophyconf")
    return text[start:] if start >= 0 else text


def _entries(blob: bytes):
    count = struct.unpack_from(">I", blob, 16)[0]
    offset = 96
    for _ in range(count):
        name = blob[offset:offset + 32].split(b"\0", 1)[0].decode()
        pos, length = struct.unpack_from(">QQ", blob, offset + 32)
        yield name, blob[pos:pos + length]
        offset += 64


def read_trp(path: Path) -> dict:
    blob = Path(path).read_bytes()
    files = dict(_entries(blob))
    words = _communication_key(files["TROP.ESFM"])
    parsed = {name: ET.fromstring(_decrypt_esfm(files[name], words)) for name, _code, _label in _LANGUAGES}
    conf = ET.fromstring(_decrypt_esfm(files["TROPCONF.ESFM"], words))
    rows = []
    for item in conf.findall("trophy"):
        number = item.attrib["id"]
        text = {}
        for name, code, _label in _LANGUAGES:
            node = parsed[name].find(f"trophy[@id='{number}']")
            text[code] = {"name": node.findtext("name") or "", "detail": node.findtext("detail") or ""}
        rows.append({
            "id": number,
            "grade": _GRADES[item.attrib["ttype"]],
            "hidden": item.attrib.get("hidden") == "yes",
            "icon": files.get(f"TROP{number}.PNG", b""),
            "text": text,
        })
    return {
        "languages": [{"id": code, "label": label} for _name, code, label in _LANGUAGES],
        "trophies": rows,
    }


def read_unlocks(path: Path) -> dict[str, str]:
    file = Path(path)
    if not file.is_file():
        return {}
    try:
        payload = json.loads(file.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}
    unlocked = payload.get("unlocked") if isinstance(payload, dict) else None
    if not isinstance(unlocked, dict):
        return {}
    return {str(key).zfill(3): str(value) for key, value in unlocked.items() if str(value)}


def note_unlock(path: Path, trophy_id: int, when: str) -> None:
    """Record one local unlock. Does not change what the game is told."""
    number = int(trophy_id)
    if number < 0 or number > 127 or not when:
        raise ValueError("bad-id")
    file = Path(path)
    unlocks = read_unlocks(file)
    key = f"{number:03d}"
    if key not in unlocks:
        unlocks[key] = when
    file.parent.mkdir(parents=True, exist_ok=True)
    payload = {"unlocked": {key: unlocks[key] for key in sorted(unlocks)}}
    temporary = file.with_name(file.name + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False) + "\n", encoding="utf-8")
    temporary.replace(file)


def _one_line(value: str) -> str:
    return " ".join(str(value).replace("\t", " ").split())


def write_card_cache(trp: Path, folder: Path, lang: str) -> int:
    """Local card text for the overlay. Written beside the save, never into a repo."""
    source = Path(trp)
    folder = Path(folder)
    codes = {code for _name, code, _label in _LANGUAGES}
    chosen = lang if lang in codes else "en"
    stamp = folder / "stamp.txt"
    token = f"{source.stat().st_mtime_ns}:{chosen}"
    if stamp.is_file() and stamp.read_text(encoding="utf-8") == token and (folder / "index.tsv").is_file():
        return sum(1 for line in (folder / "index.tsv").read_text(encoding="utf-8").splitlines() if line)
    catalog = read_trp(source)
    folder.mkdir(parents=True, exist_ok=True)
    lines = []
    for item in catalog["trophies"]:
        text = item["text"].get(chosen) or item["text"].get("en") or {}
        letter = {"Platinum": "P", "Gold": "G", "Silver": "S", "Bronze": "B"}.get(item["grade"], "B")
        lines.append("\t".join((
            str(item["id"]).zfill(3),
            letter,
            _one_line(text.get("name") or ""),
            _one_line(text.get("detail") or ""),
        )))
        icon = item.get("icon") or b""
        if icon.startswith(b"\x89PNG"):
            (folder / f"{str(item['id']).zfill(3)}.png").write_bytes(icon)
    (folder / "index.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8")
    stamp.write_text(token, encoding="utf-8")
    return len(lines)
