"""Render the real KayKit mercenary assets with their Unreal materials.

Outputs deterministic orthographic reference PNGs and a manifest that records
the exact skeletal mesh, material instance, and resolved texture parameters.
These renders are source references for ImageGen portraits, not shipping art.
"""

from __future__ import annotations

import json
import math
from pathlib import Path
import traceback

import unreal


SIZE = 768
VIEW_ANGLES = (45, 90, 135, 180, 225, 270, 315, 0)
ROOT = (
    "/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/Charaters"
)
CHARACTERS = {
    "Barbarian": f"{ROOT}/Barbarian/SkeletalMeshes/SK_Barbarian.SK_Barbarian",
    "Barbarian_Large": (
        f"{ROOT}/Barbarian_Large/SkeletalMeshes/"
        "SK_Barbarian_Large.SK_Barbarian_Large"
    ),
    "Druid": f"{ROOT}/Druid/SkeletalMeshes/SK_Druid.SK_Druid",
    "Engineer": f"{ROOT}/Engineer/SkeletalMeshes/SK_Engineer.SK_Engineer",
    "Knight": f"{ROOT}/Knight/SkeletalMeshes/SK_Knight.SK_Knight",
    "Mage": f"{ROOT}/Mage/SkeletalMeshes/SK_Mage.SK_Mage",
    "Ranger": f"{ROOT}/Ranger/SkeletalMeshes/SK_Ranger.SK_Ranger",
    "Rogue": f"{ROOT}/Rogue/SkeletalMeshes/SK_Rogue.SK_Rogue",
    "Rogue_Hooded": (
        f"{ROOT}/Rogue_Hooded/SkeletalMeshes/"
        "SK_Rogue_Hooded.SK_Rogue_Hooded"
    ),
}
OUTPUT_ROOT = (
    Path(unreal.Paths.project_dir())
    / "outputs"
    / "mercenary_icon_refs"
    / "unreal"
)
MANIFEST_PATH = (
    Path(unreal.Paths.project_saved_dir())
    / "UI"
    / "MercenaryIconReferences"
    / "unreal_capture_manifest.json"
)


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def spawn(actor_class, location=unreal.Vector(), rotation=unreal.Rotator()):
    return actor_subsystem().spawn_actor_from_class(
        actor_class, location, rotation, transient=True
    )


def configure_lighting():
    key = spawn(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-35.0, -45.0, 0.0),
    )
    key_comp = key.get_component_by_class(unreal.DirectionalLightComponent)
    key_comp.set_editor_property("intensity", 14.0)
    key_comp.set_editor_property("light_color", unreal.Color(255, 244, 230, 255))
    key_comp.set_editor_property("cast_shadows", True)

    fill = spawn(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-12.0, 135.0, 0.0),
    )
    fill_comp = fill.get_component_by_class(unreal.DirectionalLightComponent)
    fill_comp.set_editor_property("intensity", 7.0)
    fill_comp.set_editor_property("light_color", unreal.Color(210, 225, 255, 255))
    fill_comp.set_editor_property("cast_shadows", False)
    return [key, fill]


def create_target(world):
    return unreal.RenderingLibrary.create_render_target2d(
        world,
        SIZE,
        SIZE,
        unreal.TextureRenderTargetFormat.RTF_RGBA8,
        unreal.LinearColor(0.025, 0.03, 0.04, 1.0),
        False,
    )


def build_capture(target):
    capture_actor = spawn(unreal.SceneCapture2D)
    capture = capture_actor.get_component_by_class(unreal.SceneCaptureComponent2D)
    capture.set_editor_property("texture_target", target)
    capture.set_editor_property("capture_every_frame", False)
    capture.set_editor_property("capture_on_movement", False)
    capture.set_editor_property(
        "projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC
    )
    capture.set_editor_property(
        "capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    )
    capture.set_editor_property(
        "primitive_render_mode",
        unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST,
    )
    capture.set_editor_property("always_persist_rendering_state", True)
    return capture_actor, capture


def material_report(component):
    reports = []
    for index in range(component.get_num_materials()):
        material = component.get_material(index)
        if not material:
            reports.append({"slot": index, "material": None, "textures": {}})
            continue

        material.set_force_mip_levels_to_be_resident(True, True, 120.0, 0, False)
        textures = {}
        for parameter_name in unreal.MaterialEditingLibrary.get_texture_parameter_names(
            material
        ):
            texture = None
            if isinstance(material, unreal.MaterialInstanceConstant):
                texture = (
                    unreal.MaterialEditingLibrary
                    .get_material_instance_texture_parameter_value(
                        material, parameter_name
                    )
                )
            else:
                texture = (
                    unreal.MaterialEditingLibrary
                    .get_material_default_texture_parameter_value(
                        material, parameter_name
                    )
                )
            if texture:
                texture.set_force_mip_levels_to_be_resident(120.0, 0)
                textures[str(parameter_name)] = texture.get_path_name()
        reports.append(
            {
                "slot": index,
                "material": material.get_path_name(),
                "textures": textures,
            }
        )
    return reports


def capture_character(world, capture_actor, capture, target, name, mesh_path):
    skeletal_mesh = unreal.load_object(None, mesh_path)
    if not skeletal_mesh:
        raise RuntimeError(f"Could not load {mesh_path}")

    actor = spawn(unreal.SkeletalMeshActor)
    component = actor.get_component_by_class(unreal.SkeletalMeshComponent)
    component.set_skeletal_mesh_asset(skeletal_mesh)
    component.set_editor_property("cast_shadow", True)
    component.set_editor_property("visible", True)
    component.set_visibility(True, True)

    materials = material_report(component)
    origin, extent = actor.get_actor_bounds(False, False)
    diameter = max(extent.x, extent.y, extent.z) * 2.0
    if diameter <= 0.0:
        raise RuntimeError(f"Invalid bounds for {name}: {extent}")

    capture.clear_show_only_components()
    capture.show_only_component(component)
    capture.set_editor_property("ortho_width", diameter * 1.20)

    output_dir = OUTPUT_ROOT / name
    output_dir.mkdir(parents=True, exist_ok=True)
    files = []
    base_color_files = []
    distance = diameter * 3.0
    elevation = math.radians(7.0)
    for angle in VIEW_ANGLES:
        azimuth = math.radians(angle)
        location = unreal.Vector(
            origin.x + math.cos(azimuth) * math.cos(elevation) * distance,
            origin.y + math.sin(azimuth) * math.cos(elevation) * distance,
            origin.z + math.sin(elevation) * distance,
        )
        rotation = unreal.MathLibrary.find_look_at_rotation(location, origin)
        capture_actor.set_actor_location(location, False, False)
        capture_actor.set_actor_rotation(rotation, False)
        capture.capture_scene()

        filename = f"{name}_{angle:03d}.png"
        unreal.RenderingLibrary.export_render_target(
            world, target, str(output_dir), filename
        )
        files.append(str(output_dir / filename))

        if angle in (45, 90):
            capture.set_editor_property(
                "capture_source", unreal.SceneCaptureSource.SCS_BASE_COLOR
            )
            capture.capture_scene()
            base_color_filename = f"{name}_{angle:03d}_basecolor.png"
            unreal.RenderingLibrary.export_render_target(
                world, target, str(output_dir), base_color_filename
            )
            base_color_files.append(str(output_dir / base_color_filename))
            capture.set_editor_property(
                "capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
            )

    actor_subsystem().destroy_actor(actor)
    return {
        "name": name,
        "mesh": mesh_path,
        "materials": materials,
        "bounds_origin": [origin.x, origin.y, origin.z],
        "bounds_extent": [extent.x, extent.y, extent.z],
        "files": files,
        "base_color_files": base_color_files,
    }


def main():
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_editor_world()
    if not world:
        raise RuntimeError("No editor world")

    spawned = []
    try:
        spawned.extend(configure_lighting())
        target = create_target(world)
        capture_actor, capture = build_capture(target)
        spawned.append(capture_actor)
        captures = [
            capture_character(
                world, capture_actor, capture, target, name, mesh_path
            )
            for name, mesh_path in CHARACTERS.items()
        ]
        manifest = {
            "size": SIZE,
            "angles": list(VIEW_ANGLES),
            "captures": captures,
        }
        MANIFEST_PATH.write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )
        unreal.log(
            f"[MercenaryIconRefs] captured {len(captures)} characters"
        )
    finally:
        for actor in reversed(spawned):
            if actor:
                actor_subsystem().destroy_actor(actor)


try:
    main()
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
