"""Create the gitignored config files a CI build needs.

esphome/secrets.yaml and esphome/local_entities.h are per-owner files kept out of
the repository, but the config will not validate without them. This writes
throwaway versions from the committed examples and selects a PCB revision.

Usage: prepare_ci_config.py [--pcb pcb_rev31] [--root .]
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

# Placeholder credentials only. The firmware CI builds is never flashed to a real
# device, and none of these values grant access to anything.
CI_SECRETS = """\
wifi_ssid: "ci-build-ssid"
wifi_password: "ci-build-password"
# Valid base64 for a 32-byte key so the api encryption schema accepts it.
encryption_key: "GLFRuKZjuVGxKrEXBKGdEbHrTVLNoOoTNQ7CPHqLPHo="
ota_password: "ci-build-ota-password"
alarm_code: "0000"
web_server_username: "ci-build"
web_server_password: "ci-build"
"""

PCB_CHOICES = ("pcb_rev1", "pcb_rev2", "pcb_rev31")


def _select_pcb(settings_path: Path, pcb: str) -> None:
    """Point the select_pcb package at the requested revision.

    settings.yaml keeps every revision on its own line with all but one
    commented out, so rewrite those lines rather than the whole block.
    """
    text = settings_path.read_text(encoding="utf-8")
    updated_lines = []
    matched = False
    for line in text.splitlines():
        match = re.match(r"^(\s*)#?\s*file:\s*packages/(pcb_rev\w+)\.yaml\s*$", line)
        if match:
            indent, revision = match.groups()
            if revision == pcb:
                updated_lines.append(f"{indent}file: packages/{pcb}.yaml")
                matched = True
            else:
                updated_lines.append(f"{indent}#file: packages/{revision}.yaml")
        else:
            updated_lines.append(line)

    if not matched:
        raise SystemExit(
            f"No 'file: packages/{pcb}.yaml' line found in {settings_path}. "
            "Available revisions must be listed there, commented or not."
        )

    settings_path.write_text("\n".join(updated_lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pcb", choices=PCB_CHOICES, default="pcb_rev31")
    parser.add_argument("--root", type=Path, default=Path("."))
    args = parser.parse_args()

    esphome_dir = args.root / "esphome"
    examples_dir = esphome_dir / "examples"

    entities_example = examples_dir / "local_entities-example.h"
    if not entities_example.is_file():
        raise SystemExit(f"Missing {entities_example}")

    (esphome_dir / "secrets.yaml").write_text(CI_SECRETS, encoding="utf-8")
    (esphome_dir / "local_entities.h").write_text(
        entities_example.read_text(encoding="utf-8"), encoding="utf-8"
    )
    _select_pcb(esphome_dir / "settings.yaml", args.pcb)

    print(f"Prepared CI configuration for {args.pcb}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
