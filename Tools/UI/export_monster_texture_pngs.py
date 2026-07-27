"""Export monster source textures to PNG for color-reference inspection."""

from __future__ import annotations

import json
import os

import unreal


ROOTS = {
    "EagleUnit": (
        "/Game/SVN/OutSideAsset/MMP_MonsterMegaPack_01/ma001_Eagle/Textures",
        ["T_ma001_Eagle_{}".format(i) for i in range(1, 7)],
    ),
    "GolemUnit": (
        "/Game/SVN/OutSideAsset/MMP_MonsterMegaPack_01/ms01_Golem/Textures",
        ["T_ms01_golem_Texture_{}".format(i) for i in range(1, 7)],
    ),
    "SpiderUnit": (
        "/Game/SVN/OutSideAsset/MMP_MonsterMegaPack_01/ms02_03_Spider/Textures",
        ["T_ms02_03_Spider_{}".format(i) for i in range(1, 7)],
    ),
    "WerewolfUnit": (
        "/Game/SVN/OutSideAsset/MMP_MonsterMegaPack_01/ms03_06_Werewolf/Textures",
        ["T_ms03_06_Werewolf_{}".format(i) for i in range(1, 7)],
    ),
    "LeshyUnit": (
        "/Game/SVN/OutSideAsset/MMP_MonsterMegaPack_01/ms05_Leshy/Textures",
        ["T_ms05_Leshy_{}".format(i) for i in range(1, 7)],
    ),
    "SurvivalPack": (
        "/Game/SVN/OutSideAsset/MonsterForSurvivalGame/Texture",
        ["BasecolorDefault_TEX", "Emissive_TEX", "RAM_TEX"],
    ),
    "KayKitSkeletons": (
        "/Game/SVN/OutSideAsset/Kenney/KatKit_Skeletons/Textures",
        ["T_skeleton_texture_A", "T_skeleton_texture_B"],
    ),
}
OUTPUT = os.path.join(
    unreal.Paths.project_dir(), "outputs", "monster_icon_refs", "textures"
)

os.makedirs(OUTPUT, exist_ok=True)
manifest = {}
for group, (root, names) in ROOTS.items():
    group_dir = os.path.join(OUTPUT, group)
    os.makedirs(group_dir, exist_ok=True)
    for name in names:
        path = "{}/{}".format(root, name)
        texture = unreal.EditorAssetLibrary.load_asset(path)
        if texture is None:
            raise RuntimeError("missing texture " + path)
        destination = os.path.join(group_dir, name + ".png")
        task = unreal.AssetExportTask()
        task.set_editor_property("object", texture)
        task.set_editor_property("filename", destination)
        task.set_editor_property("automated", True)
        task.set_editor_property("prompt", False)
        task.set_editor_property("replace_identical", True)
        task.set_editor_property("exporter", unreal.TextureExporterPNG())
        if not unreal.Exporter.run_asset_export_task(task):
            raise RuntimeError("texture export failed: " + path)
        manifest["{}/{}".format(group, name)] = {
            "asset": texture.get_path_name(),
            "png": destination.replace("\\", "/"),
            "bytes": os.path.getsize(destination),
        }

with open(
    os.path.join(OUTPUT, "texture_manifest.json"), "w", encoding="utf-8"
) as handle:
    json.dump(manifest, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] exported {} source textures".format(len(manifest)))
