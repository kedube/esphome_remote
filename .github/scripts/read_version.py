"""Print the current VERSION substitution from esphome/settings.yaml.

Used by .github/workflows/release.yml when re-releasing without a bump.

Usage: read_version.py [settings_path]
"""

from __future__ import annotations

from pathlib import Path
import re
import sys

VERSION_RE = re.compile(r'^\s*VERSION:\s*"([^"]+)"', re.MULTILINE)


def main() -> int:
    settings_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("esphome/settings.yaml")
    match = VERSION_RE.search(settings_path.read_text(encoding="utf-8"))
    if match is None:
        raise SystemExit(f"Could not find a VERSION: \"...\" line in {settings_path}")
    print(match.group(1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
