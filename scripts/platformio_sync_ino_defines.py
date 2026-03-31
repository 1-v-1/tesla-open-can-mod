from pathlib import Path

from SCons.Errors import UserError
from SCons.Script import Import

Import("env")


DRIVER_DEFINES = ("DRIVER_MCP2515", "DRIVER_SAME51", "DRIVER_TWAI")
VEHICLE_DEFINES = ("LEGACY", "HW3", "HW4")
OPTIONAL_DEFINES = ("ISA_SPEED_CHIME_SUPPRESS", "EMERGENCY_VEHICLE_DETECTION")


def _active_defines(text):
    active = set()
    for line in text.splitlines():
        stripped = line.lstrip()
        if not stripped.startswith("#define"):
            continue
        parts = stripped.split(None, 2)
        if len(parts) >= 2:
            active.add(parts[1])
    return active


def _pick_one(active, choices, label):
    selected = [name for name in choices if name in active]
    if len(selected) != 1:
        raise UserError(
            f"RP2040CAN.ino must enable exactly one {label}: {', '.join(choices)}."
        )
    return selected[0]


def _normalize_cppdefines(raw_defines):
    normalized = set()
    for item in raw_defines or []:
        if isinstance(item, (tuple, list)):
            if item:
                normalized.add(item[0])
        else:
            normalized.add(item)
    return normalized


def _project_option_defines(env_obj):
    define_sources = []

    build_flags_option = env_obj.GetProjectOption("build_flags", "")
    if isinstance(build_flags_option, str):
        define_sources.append(build_flags_option)
    else:
        define_sources.extend(build_flags_option)

    substituted_build_flags = env_obj.subst("$BUILD_FLAGS").strip()
    if substituted_build_flags:
        define_sources.append(substituted_build_flags)

    normalized = set()
    for source in define_sources:
        parsed = env_obj.ParseFlags(source)
        normalized.update(_normalize_cppdefines(parsed.get("CPPDEFINES")))
    return normalized


project_dir = Path(env["PROJECT_DIR"])
sketch_path = project_dir / "RP2040CAN.ino"
active = _active_defines(sketch_path.read_text(encoding="utf-8"))

selected_driver = _pick_one(active, DRIVER_DEFINES, "driver define")
selected_vehicle = _pick_one(active, VEHICLE_DEFINES, "vehicle define")
selected_options = [name for name in OPTIONAL_DEFINES if name in active]

env_defines = _normalize_cppdefines(env.get("CPPDEFINES"))
project_defines = _project_option_defines(env)
env_driver = [name for name in DRIVER_DEFINES if name in project_defines]
env_vehicle = [name for name in VEHICLE_DEFINES if name in env_defines]

if len(env_driver) != 1:
    raise UserError(
        f"PlatformIO env '{env['PIOENV']}' must define exactly one CAN driver: "
        f"{', '.join(DRIVER_DEFINES)}."
    )

if env_driver[0] != selected_driver:
    raise UserError(
        f"RP2040CAN.ino selects {selected_driver}, but PlatformIO env "
        f"'{env['PIOENV']}' is configured for {env_driver[0]}. Pick the matching "
        f"'pio run -e ...' environment or update RP2040CAN.ino."
    )

if env_vehicle and env_vehicle != [selected_vehicle]:
    raise UserError(
        f"PlatformIO env '{env['PIOENV']}' already defines {env_vehicle[0]}, but "
        f"RP2040CAN.ino selects {selected_vehicle}. Remove the conflicting build flag."
    )

sync_defines = [selected_vehicle, *selected_options]
missing_defines = [name for name in sync_defines if name not in env_defines]
if missing_defines:
    env.Append(CPPDEFINES=missing_defines)

print(
    f"Synced RP2040CAN.ino defines for {env['PIOENV']}: "
    f"{selected_vehicle}"
    + (f", {', '.join(selected_options)}" if selected_options else "")
)
