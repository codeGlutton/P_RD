"""Import one exported monster GLB into a clean Blender file and save it.

Run with Blender in background mode:
    blender --background --python import_monster_glb_to_blend.py -- \
        --input path/to/monster.glb --output path/to/monster.blend
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

import bpy


def script_args() -> list[str]:
    return sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(script_args())

    source = Path(args.input).resolve()
    destination = Path(args.output).resolve()
    if not source.is_file():
        raise FileNotFoundError(source)
    destination.parent.mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))

    # Some locally installed Blender add-ons create a material-less helper
    # Icosphere when factory settings are reloaded. It is not part of the GLB
    # and would distort bounds/camera framing if saved with the monster.
    for obj in list(bpy.context.scene.objects):
        if (
            obj.type == "MESH"
            and obj.name.startswith("Icosphere")
            and not any(slot.material for slot in obj.material_slots)
        ):
            bpy.data.objects.remove(obj, do_unlink=True)

    if bpy.context.scene.world is None:
        bpy.context.scene.world = bpy.data.worlds.new("World")

    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not mesh_objects:
        raise RuntimeError(f"No mesh objects imported from {source}")

    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    for obj in mesh_objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_objects[0]

    bpy.ops.wm.save_as_mainfile(filepath=str(destination), compress=True)
    print(
        json.dumps(
            {
                "source": str(source),
                "output": str(destination),
                "mesh_objects": [obj.name for obj in mesh_objects],
                "materials": sorted(
                    {
                        slot.material.name
                        for obj in mesh_objects
                        for slot in obj.material_slots
                        if slot.material
                    }
                ),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
