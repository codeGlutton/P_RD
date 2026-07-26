"""Export KayKit mercenaries with their original Interchange glTF materials."""

from __future__ import annotations

import json
import os

import unreal


ROOT = (
    "/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/Charaters"
)
CHARACTERS = {
    "Barbarian": f"{ROOT}/Barbarian/SkeletalMeshes/SK_Barbarian",
    "Barbarian_Large": (
        f"{ROOT}/Barbarian_Large/SkeletalMeshes/SK_Barbarian_Large"
    ),
    "Druid": f"{ROOT}/Druid/SkeletalMeshes/SK_Druid",
    "Engineer": f"{ROOT}/Engineer/SkeletalMeshes/SK_Engineer",
    "Knight": f"{ROOT}/Knight/SkeletalMeshes/SK_Knight",
    "Mage": f"{ROOT}/Mage/SkeletalMeshes/SK_Mage",
    "Ranger": f"{ROOT}/Ranger/SkeletalMeshes/SK_Ranger",
    "Rogue": f"{ROOT}/Rogue/SkeletalMeshes/SK_Rogue",
    "Rogue_Hooded": (
        f"{ROOT}/Rogue_Hooded/SkeletalMeshes/SK_Rogue_Hooded"
    ),
}
PROJECT = unreal.Paths.project_dir()
OUT_DIR = os.path.join(PROJECT, "work", "mercenary_icon_refs", "glb")
MANIFEST = os.path.join(
    PROJECT, "work", "mercenary_icon_refs", "export_manifest.json"
)

os.makedirs(OUT_DIR, exist_ok=True)

options = unreal.GLTFExportOptions()
# These assets came from the Interchange glTF importer. Direct mapping retains
# the imported material/texture assignments instead of trying to approximate
# their material instance graph.
# The imported material mapping drops this pack's palette-atlas sample when
# exporting the SkeletalMesh as a standalone asset. Bake with the actual mesh
# UV data so the palette texture becomes a regular glTF base-color texture.
options.set_editor_property("use_importer_material_mapping", False)
options.set_editor_property(
    "bake_material_inputs", unreal.GLTFMaterialBakeMode.USE_MESH_DATA
)
options.set_editor_property(
    "default_material_bake_size",
    unreal.GLTFMaterialBakeSize(x=1024, y=1024, auto_detect=False),
)
options.set_editor_property("export_vertex_colors", True)
options.set_editor_property(
    "texture_image_format", unreal.GLTFTextureImageFormat.PNG
)
options.set_editor_property("texture_image_quality", 100)
options.set_editor_property("adjust_normalmaps", True)

result = {}
for name, package in CHARACTERS.items():
    mesh = unreal.EditorAssetLibrary.load_asset(package)
    if not mesh:
        raise RuntimeError(f"Could not load {package}")
    output = os.path.join(OUT_DIR, f"{name}.glb")
    messages = unreal.GLTFExporter.export_to_gltf(mesh, output, options, set())
    if messages is None or not os.path.isfile(output):
        raise RuntimeError(f"glTF export failed: {package}")
    result[name] = {
        "mesh": mesh.get_path_name(),
        "glb": output.replace("\\", "/"),
        "bytes": os.path.getsize(output),
    }
    unreal.log(f"[MercenaryRefs] exported {name} -> {output}")

with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log(f"[MercenaryRefs] exported {len(result)} GLBs")
