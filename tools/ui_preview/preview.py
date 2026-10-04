#!/usr/bin/env python3
"""Render the remote's OLED UI on your computer, without flashing the remote.

Compiles src/remote_ui_renderer.cpp together with ESPHome's own display and
font code (taken from the installed esphome package) and the font bitmaps
ESPHome generates from esphome/packages/remote_fonts.yaml, then draws every
sample state in scenarios.cpp. What you see is pixel for pixel what the
128x64 panel shows, in the fonts chosen in esphome/settings.yaml (--readme
uses the defaults from esphome/examples/settings-example.yaml).

Run it from anywhere in the repo (it re-runs itself under the Python behind
your `esphome` command, so it uses the same ESPHome you build with):

    python3 tools/ui_preview/preview.py              # contact sheet of every screen
    python3 tools/ui_preview/preview.py --frames DIR # also one PNG per screen
    python3 tools/ui_preview/preview.py --readme     # also refresh images/remote_UI-*.png
    python3 tools/ui_preview/preview.py --stats      # pixel work per frame
    python3 tools/ui_preview/preview.py --stress     # also draw hostile input under sanitizers

--stress builds stress.cpp with AddressSanitizer and UndefinedBehaviorSanitizer
and draws every mode and setting with broken text and NaN or out-of-range
numbers. It exits non-zero on the first sanitizer report; CI runs it on every
change.

Needs a C++20 compiler (clang++ or g++), and network access the first time so
ESPHome can download the Google fonts. Build files are kept in .cache/ here.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
CACHE = HERE / ".cache"

PANEL_COLOR = (120, 220, 255)  # lit pixel colour of the blue SH1106 panels
SCALE = 3

# Scenario (frame) names used for the README screenshot table, in order.
README_SHOTS = [
    "01_light_on",
    "06_climate_heat",
    "07_climate_dual",
    "10_humidifier",
    "11_fan",
    "13_cover_position",
    "15_lock_unlocked",
    "16_lock_holding",
    "18_media_playing",
    "20_media_tv",
    "04_switch_on",
    "22_sensor_temp",
    "68_automation_hold",
    "27_alarm",
    "29_notification",
    "31_weather",
    "32_weather_wind",
    "34_info_time",
    "35_info_wifi",
    "45_sys_hold_reboot",
    "a8_vacuum_cleaning",
    "ab_timer_active",
    "a6_number",
    "a7_select",
    "al_snapshot_api",
    "b2_remote_down",
    "b7_event_doorbell",
    "ba_lock_open_setting",
    "bc_light_color",
    "al_snapshot_queued",
]

# Compiler flags for --stress. float-cast-overflow (which catches NaN cast to
# int) is named on its own because GCC leaves it out of "undefined", and vptr is
# off because it needs RTTI, which ESPHome builds without.
SANITIZE = [
    "-g",
    "-fno-omit-frame-pointer",
    "-fsanitize=address,undefined,float-cast-overflow",
    "-fno-sanitize=vptr",
    "-fno-sanitize-recover=all",
]

DEFINES_H = """#pragma once
#include "esphome/core/macros.h"
#define USE_HOST
#define USE_DISPLAY
#define USE_FONT
#define ESPHOME_COMPONENT_COUNT 1
#define ESPHOME_VARIANT "HOST"
#define ESPHOME_BOARD "host"
"""


def esphome_python() -> str:
    """The Python behind the `esphome` command, or this one if it cannot be found.

    Only a shebang that names a Python counts: pip writes a /bin/sh trampoline
    when the path has spaces or is very long, and `#!/usr/bin/env python3`
    names env, not the interpreter.
    """
    exe = shutil.which("esphome")
    if exe is not None:
        try:
            first_line = Path(exe).read_bytes()[:512].split(b"\n", 1)[0]
        except OSError:
            first_line = b""
        if first_line.startswith(b"#!"):
            words = first_line[2:].decode(errors="ignore").split()
            if len(words) > 1 and Path(words[0]).name == "env":
                words = [shutil.which(words[1]) or ""]
            if words and Path(words[0]).name.startswith("python") and Path(words[0]).is_file():
                return words[0]
    return sys.executable


def run(cmd, **kwargs):
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        sys.exit(f"command failed: {' '.join(str(c) for c in cmd)}")
    return result.stdout


def copy_esphome_sources(work: Path) -> None:
    import esphome  # pylint: disable=import-outside-toplevel

    src = Path(esphome.__file__).resolve().parent
    # Headers only beyond the four files build() compiles; newer ESPHome keeps
    # the host HAL under components/host and splits core into subdirectories.
    keep = shutil.ignore_patterns("*.py", "*.pyc", "__pycache__")
    for sub in ("core", "components/display", "components/font", "components/host"):
        if (src / sub).is_dir():
            shutil.copytree(src / sub, work / "esphome" / sub, ignore=keep, dirs_exist_ok=True)
    (work / "esphome" / "core" / "defines.h").write_text(DEFINES_H)


# The settings lines that choose the text fonts and their sizes.
FONT_SETTING_RE = re.compile(
    r"""^[ \t]+((?:LABEL|TEXT|NAME|STATE|VALUE)_FONT(?:_SIZE|_SMALL_SIZE)?):[ \t]*(?:"([^"]*)"|'([^']*)'|([^\s#]+))""",
    re.M,
)


def font_settings(use_settings: bool) -> dict:
    """The font substitutions remote_fonts.yaml needs.

    Read from esphome/examples/settings-example.yaml, then from
    esphome/settings.yaml when use_settings is true, so the preview draws the
    fonts your remote uses. Local font paths are made absolute, because the
    preview builds the fonts in another directory.
    """
    sources = [REPO / "esphome" / "examples" / "settings-example.yaml"]
    if use_settings:
        sources.append(REPO / "esphome" / "settings.yaml")
    found = {}
    for path in sources:
        for match in FONT_SETTING_RE.finditer(path.read_text(encoding="utf-8")):
            found[match.group(1)] = next(value for value in match.groups()[1:] if value is not None)
    for key, value in found.items():
        if key.endswith("_FONT") and not value.startswith(("gfonts://", "http://", "https://")):
            found[key] = (REPO / "esphome" / value).resolve().as_posix()
    return found


def generate_fonts(work: Path, substitutions: dict) -> None:
    """Has ESPHome rasterise remote_fonts.yaml, then lifts the result out of main.cpp."""
    root = CACHE / "fontgen"
    config_dir = root / "esphome"
    config_dir.mkdir(parents=True, exist_ok=True)
    config = config_dir / "ui_preview_fonts.yaml"
    fonts_yaml = (REPO / "esphome" / "packages" / "remote_fonts.yaml").as_posix()
    # JSON strings are valid double-quoted YAML scalars.
    subs = "".join(f"  {key}: {json.dumps(value)}\n" for key, value in substitutions.items())
    config.write_text(
        "esphome:\n  name: ui-preview-fonts\nhost:\n"
        + (f"substitutions:\n{subs}" if subs else "")
        + f"packages:\n  fonts: !include {fonts_yaml}\n"
    )
    run([sys.executable, "-m", "esphome", "compile", "--only-generate", str(config)])

    main_cpp = (config_dir / ".esphome" / "build" / "ui-preview-fonts" / "src" / "main.cpp").read_text()
    # Matches both the older "tiny_font = new font::Font(...)" codegen and the
    # newer placement-new form.
    bitmaps = re.findall(r"^\s*static (?:constexpr|const) uint8_t (uint8_t_id\w*)\[\] PROGMEM = (\{.*?\});$",
                         main_cpp, re.M)
    glyphs = re.findall(r"^\s*static const font::Glyph (font_glyph_id\w*)\[\] = (\{.*?\});$", main_cpp, re.M)
    fonts = re.findall(r"^\s*new\((\w+)\) font::Font\((.*?)\);$", main_cpp, re.M)
    fonts += re.findall(r"^\s*(\w+) = new font::Font\((.*?)\);$", main_cpp, re.M)
    if not fonts:
        sys.exit("no fonts found in the generated code; has ESPHome's font codegen changed?")
    out = ['#include <cstring>', '#include "esphome/components/font/font.h"', "using namespace esphome;"]
    out += [f"static const uint8_t {name}[] = {body};" for name, body in bitmaps]
    out += [f"static const font::Glyph {name}[] = {body};" for name, body in glyphs]
    out.append("font::Font *sim_font(const char *name) {")
    for name, args in fonts:
        out.append(f"  static font::Font {name}_obj({args});")
        out.append(f'  if (strcmp(name, "{name}") == 0) return &{name}_obj;')
    out.append("  return nullptr;\n}")
    (work / "fonts_gen.cpp").write_text("\n".join(out) + "\n")


def undefined_esphome_symbols(objects) -> list:
    """ESPHome core symbols the objects reference but none of them defines."""
    defined, undefined = set(), set()
    for obj in objects:
        for line in run(["nm", "-g", str(obj)]).splitlines():
            parts = line.split()
            if len(parts) == 2 and parts[0] == "U":
                undefined.add(parts[1])
            elif len(parts) == 3 and parts[1] != "U":
                defined.add(parts[2])
    return sorted(sym for sym in undefined - defined if "7esphome" in sym)


def write_stubs(symbols, path: Path) -> None:
    """Defines each missing ESPHome core symbol as an inert placeholder.

    The display and font code reference parts of the core (component lifecycle,
    the Application object) that drawing a frame never touches, and their
    signatures change between ESPHome versions. Defining them by mangled name
    keeps the preview independent of those signatures. Only progmem_read_byte,
    which font rendering calls, gets a real body.
    """
    lines = ["// Generated by preview.py. Do not edit.", "#include <cstdint>", 'extern "C" {']
    for sym in symbols:
        name = sym[1:] if sys.platform == "darwin" else sym  # C adds the leading underscore on macOS
        if "progmem_read_byte" in name:
            lines.append(f"uint8_t {name}(const uint8_t *p) {{ return *p; }}")
        elif re.fullmatch(r"_ZN\w+E", name) and not name.startswith("_ZNK"):
            lines.append(f"alignas(64) char {name}[65536];")  # a variable such as esphome::App
        else:
            lines.append(f"void {name}() {{}}")
    lines.append("}")
    path.write_text("\n".join(lines) + "\n")


def build(work: Path, main_source: Path, name: str, extra_flags=()) -> Path:
    """Links main_source with the renderer and ESPHome's display and font code into work/name."""
    cxx = os.environ.get("CXX") or shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")
    if cxx is None:
        sys.exit("no C++ compiler found; install clang++ or g++, or set CXX")
    esp = work / "esphome" / "components"
    sources = [
        main_source,
        work / "fonts_gen.cpp",
        REPO / "src" / "remote_ui_renderer.cpp",
        esp / "display" / "display.cpp",
        esp / "display" / "display_buffer.cpp",
        esp / "display" / "rect.cpp",
        esp / "font" / "font.cpp",
    ]
    flags = ["-std=gnu++20", "-O1", "-w", "-fno-rtti", *extra_flags,
             f"-I{work}", f"-I{HERE}", f"-I{REPO / 'include'}"]
    obj_dir = work / f"{name}_obj"
    obj_dir.mkdir()
    objects = []
    for source in sources:
        obj = obj_dir / f"{source.stem}.o"
        run([cxx, *flags, "-c", str(source), "-o", str(obj)])
        objects.append(obj)
    stubs = obj_dir / "stubs.cpp"
    write_stubs(undefined_esphome_symbols(objects), stubs)
    run([cxx, *flags, "-c", str(stubs), "-o", str(obj_dir / "stubs.o")])
    binary = work / name
    run([cxx, *extra_flags, *map(str, objects), str(obj_dir / "stubs.o"), "-o", str(binary)])
    return binary


def panel_image(pbm: Path, scale: int):
    from PIL import Image  # pylint: disable=import-outside-toplevel

    frame = Image.open(pbm).convert("L")  # PBM: 1 (lit) reads as black
    lit = frame.point(lambda p: 255 if p < 128 else 0).resize((128 * scale, 64 * scale), Image.NEAREST)
    image = Image.new("RGB", lit.size, (0, 0, 0))
    image.paste(PANEL_COLOR, mask=lit)
    return image


def contact_sheet(frames, path: Path, columns: int = 4) -> None:
    from PIL import Image, ImageDraw  # pylint: disable=import-outside-toplevel

    pad, label = 10, 14
    w, h = 128 * SCALE, 64 * SCALE
    rows = (len(frames) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * (w + pad) + pad, rows * (h + pad + label) + pad), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    for i, pbm in enumerate(frames):
        x = pad + (i % columns) * (w + pad)
        y = pad + (i // columns) * (h + pad + label)
        draw.text((x, y), pbm.stem, fill=(255, 220, 90))
        sheet.paste(panel_image(pbm, SCALE), (x, y + label))
    sheet.save(path)


def main() -> None:
    interpreter = esphome_python()
    if Path(interpreter).resolve() != Path(sys.executable).resolve() and "UI_PREVIEW_CHILD" not in os.environ:
        os.environ["UI_PREVIEW_CHILD"] = "1"
        os.execv(interpreter, [interpreter, str(Path(__file__).resolve()), *sys.argv[1:]])

    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--frames", type=Path, help="also write one PNG per screen into this directory")
    parser.add_argument("--readme", action="store_true", help="refresh the README screenshots in images/")
    parser.add_argument("--stats", action="store_true", help="print the drawing work each frame needs")
    parser.add_argument("--stress", action="store_true",
                        help="also draw every screen with hostile input under sanitizers; fails on any report")
    args = parser.parse_args()

    work = CACHE / "build"
    if work.exists():
        shutil.rmtree(work)
    (work / "frames").mkdir(parents=True)

    copy_esphome_sources(work)
    # The README screenshots show the example's fonts, whatever settings.yaml picks.
    use_settings = not args.readme and (REPO / "esphome" / "settings.yaml").is_file()
    if use_settings:
        print("fonts from esphome/settings.yaml")
    generate_fonts(work, font_settings(use_settings))
    binary = build(work, HERE / "scenarios.cpp", "ui_preview")
    output = run([str(binary), str(work / "frames")] + (["--stats"] if args.stats else []))
    if args.stats:
        print(output, end="")
    frames = sorted((work / "frames").glob("*.pbm"))

    sheet = CACHE / "ui_preview.png"
    contact_sheet(frames, sheet)
    print(f"{len(frames)} screens -> {sheet}")

    if args.frames:
        args.frames.mkdir(parents=True, exist_ok=True)
        for pbm in frames:
            panel_image(pbm, SCALE).save(args.frames / f"{pbm.stem}.png")
        print(f"frames -> {args.frames}")

    if args.readme:
        by_name = {pbm.stem: pbm for pbm in frames}
        for number, name in enumerate(README_SHOTS, start=1):
            panel_image(by_name[name], SCALE).save(REPO / "images" / f"remote_UI-{number}.png")
        print(f"README screenshots -> {REPO / 'images'}")

    # Last, so the contact sheet exists even when this fails.
    if args.stress:
        stress = build(work, HERE / "stress.cpp", "ui_stress", SANITIZE)
        # No leak checking: macOS has no leak sanitizer, and a pass should mean
        # the same thing on every machine.
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="print_stacktrace=1")
        print(run([str(stress)], env=env), end="")


if __name__ == "__main__":
    main()
