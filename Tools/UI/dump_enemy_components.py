"""List visual components on gameplay enemy Blueprint defaults."""

from __future__ import annotations

import json
import os

import unreal


ROOT = "/Game/BP/Pawn/Enemy"
OUTPUT = os.path.join(
    unreal.Paths.project_saved_dir(),
    "UI",
    "MonsterIconReferences",
    "enemy_components.json",
)
result = {}
for asset_path in unreal.EditorAssetLibrary.list_assets(ROOT, True, False):
    package = str(asset_path).split(".", 1)[0]
    stem = package.rsplit("/", 1)[-1]
    if not stem.startswith("BP_") or not stem.endswith("Unit"):
        continue
    generated = unreal.EditorAssetLibrary.load_blueprint_class(package)
    if generated is None:
        continue
    cdo = unreal.get_default_object(generated)
    if not isinstance(cdo, unreal.Actor):
        continue
    rows = []
    for component in cdo.get_components_by_class(unreal.ActorComponent):
        row = {
            "name": component.get_name(),
            "class": component.get_class().get_name(),
        }
        if isinstance(component, unreal.SkeletalMeshComponent):
            mesh = component.get_editor_property("skeletal_mesh")
            row["asset"] = mesh.get_path_name() if mesh else None
        elif isinstance(component, unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
            row["asset"] = mesh.get_path_name() if mesh else None
        rows.append(row)
    result[package] = rows

os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
with open(OUTPUT, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] wrote component report for {} actors".format(len(result)))
