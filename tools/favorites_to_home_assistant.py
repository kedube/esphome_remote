#!/usr/bin/env python3
"""Print the favorite lists in local_entities.h as a Home Assistant template sensor.

The remote can take its favorite lists from Home Assistant instead of the
firmware (see Favorites from Home Assistant in the README). This starts that
sensor from the lists you already have: paste what it prints into Home
Assistant's configuration.yaml, or save it as a package.

Usage: favorites_to_home_assistant.py [path/to/local_entities.h]
       (default: esphome/local_entities.h)
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

REPO = Path(__file__).resolve().parent.parent

STRING = r'"((?:[^"\\]|\\.)*)"'
ENTRY = re.compile(r"\{\s*" + STRING + r"\s*,\s*" + STRING + r"\s*(?:,\s*(?:" + STRING + r"|nullptr)\s*)?\}")
BRACED = re.compile(r"\{[^{}]*\}")
ARRAY = re.compile(r"FavoriteEntity\s+(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", re.S)
LISTS = re.compile(r"FavoriteList\s+FAVORITE_LISTS\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", re.S)
MADE_LIST = re.compile(r"make_favorite_list\s*\(\s*" + STRING + r"\s*,\s*(\w+)\s*\)")
EMPTY_LIST = re.compile(r"\{\s*" + STRING + r"\s*,\s*nullptr\s*,\s*0\s*\}")
SIMPLE_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", '"': '"', "'": "'", "\\": "\\", "?": "?", "a": "\a", "b": "\b",
                  "f": "\f", "v": "\v"}


def strip_comments(source: str) -> str:
    """Drops // and /* */ comments, leaving string literals alone."""
    out = []
    i = 0
    while i < len(source):
        if source[i] == '"':
            end = i + 1
            while end < len(source) and source[end] != '"':
                end += 2 if source[end] == "\\" else 1
            out.append(source[i : end + 1])
            i = end + 1
        elif source.startswith("//", i):
            i = source.find("\n", i)
            i = len(source) if i < 0 else i
        elif source.startswith("/*", i):
            i = source.find("*/", i + 2)
            i = len(source) if i < 0 else i + 2
        else:
            out.append(source[i])
            i += 1
    return "".join(out)


def unescape(text: str) -> str:
    """A C string literal's text. \\x and octal escapes are bytes of UTF-8."""
    out = bytearray()
    i = 0
    while i < len(text):
        c = text[i]
        if c != "\\" or i + 1 == len(text):
            out += c.encode("utf-8")
            i += 1
            continue
        e = text[i + 1]
        if e == "x":
            digits = re.match(r"[0-9a-fA-F]+", text[i + 2 :]).group(0)
            out.append(int(digits, 16) & 0xFF)
            i += 2 + len(digits)
        elif e in "01234567":
            digits = re.match(r"[0-7]{1,3}", text[i + 1 :]).group(0)
            out.append(int(digits, 8) & 0xFF)
            i += 1 + len(digits)
        elif e in "uU":
            length = 4 if e == "u" else 8
            out += chr(int(text[i + 2 : i + 2 + length], 16)).encode("utf-8")
            i += 2 + length
        else:
            out += SIMPLE_ESCAPES.get(e, e).encode("utf-8")
            i += 2
    return out.decode("utf-8", "replace")


def split_items(body: str) -> list[str]:
    """The comma-separated items of an initializer list, outside strings and brackets."""
    items, depth, start, i = [], 0, 0, 0
    while i < len(body):
        c = body[i]
        if c == '"':
            i += 1
            while i < len(body) and body[i] != '"':
                i += 2 if body[i] == "\\" else 1
        elif c in "({":
            depth += 1
        elif c in ")}":
            depth -= 1
        elif c == "," and depth == 0:
            items.append(body[start:i].strip())
            start = i + 1
        i += 1
    items.append(body[start:].strip())
    return [item for item in items if item]


def read_lists(path: Path) -> list[tuple[str, list[tuple[str, ...]]]]:
    source = strip_comments(path.read_text(encoding="utf-8"))
    arrays = {}
    for name, body in ARRAY.findall(source):
        entries = []
        for item in BRACED.findall(body):
            match = ENTRY.fullmatch(item.strip())
            if match is None:
                sys.exit(f"{path}: can't read this favorite in {name}: {item.strip()}")
            entries.append(tuple(unescape(field) for field in match.groups() if field is not None))
        arrays[name] = entries
    lists_match = LISTS.search(source)
    if lists_match is None:
        sys.exit(f"{path}: no FAVORITE_LISTS found")
    lists = []
    for item in split_items(lists_match.group(1)):
        if EMPTY_LIST.fullmatch(item):
            continue  # an empty list: the remote skips it anyway
        match = MADE_LIST.fullmatch(item)
        if match is None:
            sys.exit(f"{path}: can't read this entry in FAVORITE_LISTS: {item}")
        title, array = unescape(match.group(1)), match.group(2)
        if array not in arrays:
            sys.exit(f"{path}: FAVORITE_LISTS names {array}, which isn't defined")
        lists.append((title, arrays[array]))
    return lists


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("path", nargs="?", type=Path, default=REPO / "esphome" / "local_entities.h")
    args = parser.parse_args()

    lines = []
    for title, entries in read_lists(args.path):
        lines.append(f"#{title}")
        lines.extend("|".join(entry) for entry in entries)
    for line in lines:
        # Home Assistant reads the attribute as a template.
        if "{{" in line or "{%" in line or "{#" in line:
            print(f"warning: {line!r} looks like a template to Home Assistant; rename it", file=sys.stderr)

    print("template:")
    print("  - sensor:")
    print("      - name: Remote Favorites")
    print("        unique_id: remote_favorites")
    print("        icon: mdi:star")
    print('        state: "ok"')
    print("        attributes:")
    print("          lists: |")
    for line in lines:
        print(f"            {line}")


if __name__ == "__main__":
    main()
