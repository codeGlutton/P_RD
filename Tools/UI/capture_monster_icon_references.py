"""Render real project monster meshes to square PNG reference views in Unreal.

This intentionally renders the source SkeletalMesh assets with their original
Unreal materials. The resulting images are visual references for ImageGen, not
shipping UI assets.
"""

from __future__ import annotations

import json
import math
import os
from pathlib import Path
import traceback

import unreal


SIZE = 768
VIEW_ANGLES = tuple(range(0, 360, 45))
OUTPUT_ROOT = (
    Path(unreal.Paths.project_dir())
    / "outputs"
    / "monster_icon_refs"
    / "unreal"
)
MANIFEST_PATH = (
    Path(unreal.Paths.project_saved_dir())
    / "UI"
    / "MonsterIconReferences"
    / "unreal_capture_manifest.json"
)
SOURCE_JSON = (
    Path(unreal.Paths.project_saved_dir())
    / "UI"
    / "MonsterIconReferences"
    / "monster_meshes.json"
)


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def spawn(actor_class, location=unreal.Vector(), rotation=unreal.Rotator()):
    return actor_subsystem().spawn_actor_from_class(
        actor_class, location, rotation, transient=True
    )


def configure_lighting():
    """Create deterministic neutral key/fill lights for the reference pass."""
    key = spawn(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-35.0, -45.0, 0.0),
    )
    key_comp = key.get_component_by_class(unreal.DirectionalLightComponent)
    key_comp.set_editor_property("intensity", 12.0)
    key_comp.set_editor_property("light_color", unreal.Color(255, 244, 230, 255))
    key_comp.set_editor_property("cast_shadows", True)

    fill = spawn(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-15.0, 135.0, 0.0),
    )
    fill_comp = fill.get_component_by_class(unreal.DirectionalLightComponent)
    fill_comp.set_editor_property("intensity", 6.0)
    fill_comp.set_editor_property("light_color", unreal.Color(205, 220, 255, 255))
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


def build_capture(world, target):
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


def mesh_entries():
    source = json.loads(SOURCE_JSON.read_text(encoding="utf-8"))
    only = {
        value.strip()
        for value in os.environ.get("MONSTER_CAPTURE_ONLY", "").split(",")
        if value.strip()
    }
    entries = []
    for actor_path, components in source.items():
        actor_name = actor_path.rsplit("/", 1)[-1].removeprefix("BP_")
        if only and actor_name not in only:
            continue
        if not components:
            continue
        entries.append(
            {
                "name": actor_name,
                "actor_path": actor_path,
                "mesh_path": components[0]["mesh"],
            }
        )
    return entries


def capture_mesh(world, capture_actor, capture, target, entry):
    skeletal_mesh = unreal.load_object(None, entry["mesh_path"])
    if not skeletal_mesh:
        raise RuntimeError(f"Could not load {entry['mesh_path']}")

    mesh_actor = spawn(unreal.SkeletalMeshActor)
    mesh_component = mesh_actor.get_component_by_class(unreal.SkeletalMeshComponent)
    mesh_component.set_skeletal_mesh_asset(skeletal_mesh)
    mesh_component.set_editor_property("cast_shadow", True)
    mesh_component.set_editor_property("visible", True)
    mesh_component.set_visibility(True, True)

    origin, extent = mesh_actor.get_actor_bounds(
        only_colliding_components=False, include_from_child_actors=False
    )
    diameter = max(extent.x, extent.y, extent.z) * 2.0
    if diameter <= 0.0:
        raise RuntimeError(f"Invalid bounds for {entry['name']}: {extent}")

    capture.clear_show_only_components()
    capture.show_only_component(mesh_component)
    capture.set_editor_property("ortho_width", diameter * 1.24)

    output_dir = OUTPUT_ROOT / entry["name"]
    output_dir.mkdir(parents=True, exist_ok=True)
    files = []
    distance = diameter * 3.0
    elevation = math.radians(10.0)
    for angle in VIEW_ANGLES:
        azimuth = math.radians(angle)
        camera_location = unreal.Vector(
            origin.x + math.cos(azimuth) * math.cos(elevation) * distance,
            origin.y + math.sin(azimuth) * math.cos(elevation) * distance,
            origin.z + math.sin(elevation) * distance,
        )
        camera_rotation = unreal.MathLibrary.find_look_at_rotation(
            camera_location, origin
        )
        capture_actor.set_actor_location(camera_location, False, False)
        capture_actor.set_actor_rotation(camera_rotation, False)
        capture.capture_scene()

        stem = f"{entry['name']}_{angle:03d}"
        file_name = f"{stem}.png"
        unreal.RenderingLibrary.export_render_target(
            world, target, str(output_dir), file_name
        )
        files.append(str(output_dir / file_name))

    actor_subsystem().destroy_actor(mesh_actor)
    return {
        **entry,
        "bounds_origin": [origin.x, origin.y, origin.z],
        "bounds_extent": [extent.x, extent.y, extent.z],
        "files": files,
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
        capture_actor, capture = build_capture(world, target)
        spawned.append(capture_actor)
        results = [
            capture_mesh(world, capture_actor, capture, target, entry)
            for entry in mesh_entries()
        ]
        manifest = {
            "size": SIZE,
            "angles": list(VIEW_ANGLES),
            "captures": results,
        }
        MANIFEST_PATH.write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        unreal.log(f"[MonsterIconRefs] captured {len(results)} monsters")
    finally:
        for actor in reversed(spawned):
            if actor:
                actor_subsystem().destroy_actor(actor)


try:
    main()
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
