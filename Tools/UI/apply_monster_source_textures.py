"""Attach exported source albedo textures to monster reference .blend files."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

import bpy


def args_from_blender() -> list[str]:
    return sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--texture", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(args_from_blender())

    texture_path = Path(args.texture).resolve()
    output_path = Path(args.output).resolve()
    if not texture_path.is_file():
        raise FileNotFoundError(texture_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    image = bpy.data.images.load(str(texture_path), check_existing=False)
    image.colorspace_settings.name = "sRGB"
    updated = []
    for material in bpy.data.materials:
        material.use_nodes = True
        nodes = material.node_tree.nodes
        links = material.node_tree.links
        principled = next(
            (node for node in nodes if node.type == "BSDF_PRINCIPLED"), None
        )
        if principled is None:
            continue
        if "glow" in material.name.lower():
            glow = (0.47, 1.0, 1.0, 1.0)
            principled.inputs["Base Color"].default_value = glow
            principled.inputs["Emission Color"].default_value = glow
            principled.inputs["Emission Strength"].default_value = 2.0
            principled.inputs["Roughness"].default_value = 0.8
            updated.append({"material": material.name, "mode": "cyan_glow"})
            continue

        texture = nodes.new("ShaderNodeTexImage")
        texture.name = "SourceAlbedo"
        texture.label = texture_path.name
        texture.image = image
        links.new(texture.outputs["Color"], principled.inputs["Base Color"])
        if "Alpha" in principled.inputs:
            links.new(texture.outputs["Alpha"], principled.inputs["Alpha"])
        principled.inputs["Metallic"].default_value = 0.0
        principled.inputs["Roughness"].default_value = 0.75
        updated.append({"material": material.name, "mode": "source_albedo"})

    if not updated:
        raise RuntimeError("No materials updated")
    bpy.ops.wm.save_as_mainfile(filepath=str(output_path), compress=True)
    print(
        json.dumps(
            {
                "texture": str(texture_path),
                "output": str(output_path),
                "materials": updated,
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
