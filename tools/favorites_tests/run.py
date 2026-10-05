#!/usr/bin/env python3
"""Builds and runs the favorite-list tests on this computer.

Tests the parser for the lists Home Assistant publishes
(include/favorites_lists.h), which lists the remote uses and what it does with
new ones (src/favorites_store.cpp), that tools/favorites_to_home_assistant.py
writes the same lists the firmware builds from local_entities.h, and that
home_assistant/remote_favorites_labels.yaml builds lists the remote takes. The
C++ is built with AddressSanitizer and UndefinedBehaviorSanitizer, like the
renderer stress test.

Needs a C++20 compiler (clang++ or g++; set CXX to choose one), and PyYAML and
Jinja2, which ESPHome installs.
"""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import re

import jinja2.sandbox
import yaml

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
# As in tools/ui_preview/preview.py.
SANITIZE = ["-fsanitize=address,undefined,float-cast-overflow", "-fno-sanitize=vptr", "-fno-sanitize-recover=all",
            "-fno-omit-frame-pointer"]


def build(cxx: str, out: Path, sources: list[Path]) -> Path:
    # This folder first, so its local_entities.h is the one the tests see.
    command = [cxx, "-std=c++20", "-g", "-O1", "-Wall", "-Wextra", *SANITIZE,
               f"-I{HERE}", f"-I{REPO / 'include'}", *map(str, sources), "-o", str(out)]
    subprocess.run(command, check=True)
    return out


# Home Assistant's registries, for remote_favorites_labels.yaml: three areas
# (one with nothing labelled), an entity in no area, an entity the remote
# can't control, a name with a | in it, and one that isn't labelled.
AREAS = {"office": "Office", "kitchen": "Kitchen", "garage": "Garage"}
ENTITIES = {  # entity: (friendly name, area, labels)
    "light.office_light": ("Office Light", "office", ["remote"]),
    "light.office_recessed": ("Office Recessed", "office", ["remote"]),
    "switch.kitchen_coffee": ("Kitchen Coffee Maker", "kitchen", ["remote"]),
    "sensor.kitchen_temp": ("Temp | Kitchen", "kitchen", ["remote"]),
    "media_player.living_room_speaker": ("Speaker", "kitchen", ["remote"]),
    "camera.kitchen": ("Kitchen Camera", "kitchen", ["remote"]),
    "remote.living_room_apple_tv": ("Apple TV", None, ["remote"]),
    "light.garage": ("Garage Light", "garage", []),
}
LABELLED_LISTS = (
    "#KITCHEN\n"
    "Coffee Maker|switch.kitchen_coffee\n"
    "Speaker|media_player.living_room_speaker|Spotify|Radio|Line In\n"
    "Temp / Kitchen|sensor.kitchen_temp\n"
    "#OFFICE\n"
    "Light|light.office_light\n"
    "Recessed|light.office_recessed\n"
    "#OTHER\n"
    "Apple TV|remote.living_room_apple_tv|apple_tv"
)


def render_labelled_lists() -> str:
    """Renders the labels template's lists the way Home Assistant would, against ENTITIES."""
    config = yaml.safe_load((REPO / "home_assistant" / "remote_favorites_labels.yaml").read_text(encoding="utf-8"))
    template = config["template"][0]["sensor"][0]["attributes"]["lists"]
    env = jinja2.sandbox.ImmutableSandboxedEnvironment()
    env.tests["match"] = lambda value, pattern: re.match(pattern, value) is not None
    env.globals.update(
        areas=lambda: list(AREAS),
        area_name=AREAS.get,
        area_entities=lambda area: [e for e, (_, a, _) in ENTITIES.items() if a == area],
        label_entities=lambda label: [e for e, (_, _, labels) in ENTITIES.items() if label in labels],
        state_attr=lambda entity, attribute: ENTITIES[entity][0] if attribute == "friendly_name" else None,
    )
    return env.from_string(template).render().strip()


def main() -> None:
    cxx = os.environ.get("CXX") or shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")
    if cxx is None:
        sys.exit("no C++ compiler found; install clang++ or g++, or set CXX")
    failed = False
    with tempfile.TemporaryDirectory() as work_dir:
        work = Path(work_dir)
        lists = build(cxx, work / "test_lists", [HERE / "test_lists.cpp"])
        store = build(cxx, work / "test_store", [HERE / "test_store.cpp", REPO / "src" / "favorites_store.cpp"])
        roundtrip = build(cxx, work / "roundtrip", [HERE / "roundtrip.cpp"])

        failed |= subprocess.run([lists]).returncode != 0
        # The store logs what it does to stderr; only failures matter here.
        failed |= subprocess.run([store], stderr=subprocess.DEVNULL).returncode != 0

        converted = subprocess.run(
            [sys.executable, str(REPO / "tools" / "favorites_to_home_assistant.py"), str(HERE / "local_entities.h")],
            check=True, capture_output=True, text=True).stdout
        # The attribute as Home Assistant reads it from the YAML.
        text = yaml.safe_load(converted)["template"][0]["sensor"][0]["attributes"]["lists"]
        (work / "lists.txt").write_text(text, encoding="utf-8")
        failed |= subprocess.run([roundtrip, work / "lists.txt"]).returncode != 0

        labelled = render_labelled_lists()
        (work / "labelled.txt").write_text(labelled, encoding="utf-8")
        if labelled != LABELLED_LISTS:
            print(f"lists from labels: FAIL\n--- expected\n{LABELLED_LISTS}\n--- built\n{labelled}")
            failed = True
        elif subprocess.run([roundtrip, "--parse", work / "labelled.txt"]).returncode != 0:
            failed = True
        else:
            print("lists from labels: all passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
