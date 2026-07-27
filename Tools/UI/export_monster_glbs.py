"""Export the resolved enemy SkeletalMeshes as self-contained GLB files."""
import json
import os

import unreal


PROJECT = unreal.Paths.project_dir()
MANIFEST = os.path.join(
    unreal.Paths.project_saved_dir(), "UI", "MonsterIconReferences",
    "monster_meshes.json")
OUT_DIR = os.path.join(PROJECT, "work", "monster_icon_refs", "glb")

with open(MANIFEST, "r", encoding="utf-8") as handle:
    mapping = json.load(handle)
os.makedirs(OUT_DIR, exist_ok=True)

unique = {}
for actor_path, meshes in mapping.items():
    actor_name = actor_path.rsplit("/", 1)[-1].removeprefix("BP_")
    for entry in meshes:
        unique.setdefault(entry["mesh"], []).append(actor_name)

options = unreal.GLTFExportOptions()
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
selected_actors = set()
manifest = {}
for mesh_path, actors in sorted(unique.items()):
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if mesh is None:
        raise RuntimeError("could not load " + mesh_path)
    safe_name = sorted(actors)[0]
    output = os.path.join(OUT_DIR, safe_name + ".glb")
    messages = unreal.GLTFExporter.export_to_gltf(
        mesh, output, options, selected_actors)
    if messages is None or not os.path.exists(output):
        raise RuntimeError("glTF export failed: {} -> {}".format(
            mesh_path, output))
    manifest[safe_name] = {
        "actors": sorted(actors),
        "mesh": mesh_path,
        "glb": output.replace("\\", "/"),
        "bytes": os.path.getsize(output),
    }
    unreal.log("[MonsterRefs] exported {} -> {}".format(mesh_path, output))

out_manifest = os.path.join(
    PROJECT, "work", "monster_icon_refs", "export_manifest.json")
with open(out_manifest, "w", encoding="utf-8") as handle:
    json.dump(manifest, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] exported {} unique meshes".format(len(manifest)))
