"""Increment the VERSION substitution in esphome/settings.yaml.

Mirrors the release tags already used by this repository (2.5, 2.9, 3.0, ...):
a major.minor pair where the minor rolls over into the major at 99.

Usage: bump_version.py [settings_path] [--bump minor|major]
Prints the new version.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re

VERSION_RE = re.compile(r'^(\s*VERSION:\s*")([^"]+)(".*)$', re.MULTILINE)


def _bump_minor(version: str) -> str:
    """Return the next major.minor version, ignoring any patch segment."""
    parts = version.split(".")
    if len(parts) < 2:
        raise ValueError(
            f"Expected a version with at least major.minor parts, got: {version}"
        )

    major, minor = (int(part) for part in parts[:2])
    if minor >= 99:
        major += 1
        minor = 0
    else:
        minor += 1
    return f"{major}.{minor}"


def _bump_major(version: str) -> str:
    """Return the next major version with the minor reset to 0."""
    parts = version.split(".")
    if not parts or not parts[0].isdigit():
        raise ValueError(
            f"Expected a version with at least major.minor parts, got: {version}"
        )
    return f"{int(parts[0]) + 1}.0"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "settings", nargs="?", type=Path, default=Path("esphome/settings.yaml")
    )
    parser.add_argument("--bump", choices=("minor", "major"), default="minor")
    args = parser.parse_args()

    text = args.settings.read_text(encoding="utf-8")
    match = VERSION_RE.search(text)
    if match is None:
        raise SystemExit(f"Could not find a VERSION: \"...\" line in {args.settings}")

    bump = _bump_major if args.bump == "major" else _bump_minor
    next_version = bump(match.group(2))

    updated, replacements = VERSION_RE.subn(
        rf"\g<1>{next_version}\g<3>", text, count=1
    )
    if replacements != 1:
        raise SystemExit("Could not update the VERSION line")

    args.settings.write_text(updated, encoding="utf-8")
    print(next_version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
