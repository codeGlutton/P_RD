"""Export the exact KayKit character color textures to PNG references."""

from __future__ import annotations

import json
import os

import unreal


ROOT = "/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/Textures"
NAMES = (
    "barbarian_texture",
    "barbarian_texture_alt_A",
    "barbarian_texture_alt_B",
    "barbarian_texture_alt_C",
    "druid_texture",
    "druid_texture_alt_A",
    "druid_texture_alt_B",
    "druid_texture_alt_C",
    "engineer_texture",
    "engineer_texture_alt_A",
    "engineer_texture_alt_B",
    "engineer_texture_alt_C",
    "knight_texture",
    "knight_texture_alt_A",
    "knight_texture_alt_B",
    "knight_texture_alt_C",
    "mage_texture",
    "mage_texture_alt_A",
    "mage_texture_alt_B",
    "mage_texture_alt_C",
    "ranger_texture",
    "ranger_texture_alt_A",
    "ranger_texture_alt_B",
    "ranger_texture_alt_C",
    "rogue_texture",
    "rogue_texture_alt_A",
    "rogue_texture_alt_B",
    "rogue_texture_alt_C",
)
OUT_DIR = os.path.join(
    unreal.Paths.project_dir(), "outputs", "mercenary_icon_refs", "textures"
)
MANIFEST = os.path.join(OUT_DIR, "texture_manifest.json")

os.makedirs(OUT_DIR, exist_ok=True)
result = {}
for name in NAMES:
    path = f"{ROOT}/{name}"
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if not texture:
        raise RuntimeError(f"Missing texture {path}")
    output = os.path.join(OUT_DIR, f"{name}.png")
    task = unreal.AssetExportTask()
    task.set_editor_property("object", texture)
    task.set_editor_property("filename", output)
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("exporter", unreal.TextureExporterPNG())
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"Failed to export {path}")
    result[name] = {
        "asset": texture.get_path_name(),
        "png": output.replace("\\", "/"),
        "bytes": os.path.getsize(output),
    }

with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log(f"[MercenaryRefs] exported {len(result)} texture PNGs")
