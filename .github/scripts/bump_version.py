"""Pick the next release version and write it to the VERSION substitution in
esphome/remote_control.yaml.

Mirrors the release tags already used by this repository (2.5, 2.9, 3.0, ...):
a major.minor pair where the minor rolls over into the major at 99.

- A VERSION newer than every release tag was set by hand for this release, so
  it is used as is.
- Otherwise the newer of VERSION and the latest tag is bumped (minor, or major
  with --bump major), so a release never reuses a version that already has a
  tag, even after main was rewritten.

Usage: bump_version.py [config_path] [--bump minor|major]
Prints the new version.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess

VERSION_RE = re.compile(r'^(\s*VERSION:\s*")([^"]+)(".*)$', re.MULTILINE)
RELEASE_TAG_RE = re.compile(r"^\d+\.\d+(\.\d+)?$")


def _parse(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split("."))


def _latest_release_tag() -> str | None:
    """The highest release-style tag in the repository, if any."""
    try:
        tags = subprocess.run(
            ["git", "tag", "--list"], check=True, capture_output=True, text=True
        ).stdout.split()
    except (OSError, subprocess.CalledProcessError):
        return None
    releases = [tag for tag in tags if RELEASE_TAG_RE.match(tag)]
    return max(releases, key=_parse) if releases else None


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
        "config", nargs="?", type=Path, default=Path("esphome/remote_control.yaml")
    )
    parser.add_argument("--bump", choices=("minor", "major"), default="minor")
    args = parser.parse_args()

    text = args.config.read_text(encoding="utf-8")
    match = VERSION_RE.search(text)
    if match is None:
        raise SystemExit(f"Could not find a VERSION: \"...\" line in {args.config}")

    current = match.group(2)
    latest = _latest_release_tag()
    if latest is None or _parse(current) > _parse(latest):
        next_version = current
    else:
        bump = _bump_major if args.bump == "major" else _bump_minor
        next_version = bump(latest if _parse(latest) > _parse(current) else current)

    updated, replacements = VERSION_RE.subn(
        rf"\g<1>{next_version}\g<3>", text, count=1
    )
    if replacements != 1:
        raise SystemExit("Could not update the VERSION line")

    args.config.write_text(updated, encoding="utf-8")
    print(next_version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
