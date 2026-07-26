"""Resolve the real 3D meshes used by enemy unit Actor Blueprints."""
import json
import os

import unreal


ROOT = "/Game/BP/Pawn/Enemy"
OUT = os.path.join(
    unreal.Paths.project_saved_dir(), "UI", "MonsterIconReferences",
    "monster_meshes.json")


def object_path_to_package(path):
    return path.split(".", 1)[0]


result = {}
seen_packages = set()
for object_path in unreal.EditorAssetLibrary.list_assets(ROOT, True, False):
    path = str(object_path)
    package = object_path_to_package(path)
    if package in seen_packages:
        continue
    seen_packages.add(package)

    stem = package.rsplit("/", 1)[-1]
    if not stem.startswith("BP_") or not stem.endswith("Unit"):
        continue
    generated = unreal.EditorAssetLibrary.load_blueprint_class(package)
    if generated is None:
        continue
    cdo = unreal.get_default_object(generated)
    if not isinstance(cdo, unreal.Actor):
        continue

    meshes = []
    for component in cdo.get_components_by_class(unreal.SkeletalMeshComponent):
        mesh = component.get_editor_property("skeletal_mesh")
        if mesh is None:
            continue
        meshes.append({
            "component": component.get_name(),
            "mesh": mesh.get_path_name(),
            "mesh_name": mesh.get_name(),
        })
    if meshes:
        result[package] = meshes
        unreal.log("[MonsterRefs] {} -> {}".format(package, meshes))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] wrote {} actors to {}".format(len(result), OUT))
