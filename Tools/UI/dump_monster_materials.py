"""Record resolved material parameters for every gameplay monster mesh."""

from __future__ import annotations

import json
import os

import unreal


SOURCE = os.path.join(
    unreal.Paths.project_saved_dir(),
    "UI",
    "MonsterIconReferences",
    "monster_meshes.json",
)
OUTPUT = os.path.join(
    unreal.Paths.project_saved_dir(),
    "UI",
    "MonsterIconReferences",
    "monster_materials.json",
)


def parameter_value(material, kind, name):
    prefix = "get_material_instance" if isinstance(
        material, unreal.MaterialInstanceConstant
    ) else "get_material_default"
    function = getattr(
        unreal.MaterialEditingLibrary,
        "{}_{}_parameter_value".format(prefix, kind),
    )
    return function(material, name)


def report_material(material):
    textures = {}
    vectors = {}
    scalars = {}
    for name in unreal.MaterialEditingLibrary.get_texture_parameter_names(material):
        value = parameter_value(material, "texture", name)
        if value:
            textures[str(name)] = value.get_path_name()
    for name in unreal.MaterialEditingLibrary.get_vector_parameter_names(material):
        value = parameter_value(material, "vector", name)
        if value is not None:
            vectors[str(name)] = [value.r, value.g, value.b, value.a]
    for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material):
        value = parameter_value(material, "scalar", name)
        if value is not None:
            scalars[str(name)] = value
    return {
        "material": material.get_path_name(),
        "textures": textures,
        "vectors": vectors,
        "scalars": scalars,
    }


with open(SOURCE, "r", encoding="utf-8") as handle:
    mapping = json.load(handle)

result = {}
seen = set()
for actor_path, meshes in sorted(mapping.items()):
    for entry in meshes:
        mesh_path = entry["mesh"]
        key = "{}|{}".format(actor_path, mesh_path)
        if key in seen:
            continue
        seen.add(key)
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if mesh is None:
            raise RuntimeError("could not load " + mesh_path)
        rows = []
        for index, slot in enumerate(mesh.get_editor_property("materials")):
            material = slot.get_editor_property("material_interface")
            if material is None:
                rows.append({"slot": index, "material": None})
            else:
                row = report_material(material)
                row["slot"] = index
                rows.append(row)
        result[actor_path] = {"mesh": mesh_path, "materials": rows}

os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
with open(OUTPUT, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] wrote material report for {} actors".format(len(result)))
