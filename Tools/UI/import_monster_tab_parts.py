"""Import the transparent Marchbound monster-tab parts as UI textures."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "UI" / "Marchbound" / "MonsterTab"
DESTINATION = "/Game/UI/MonsterTab/Textures"
PARTS = (
    ("T_MT_BaseFrame.png", "T_MT_BaseFrame"),
    ("T_MT_RowNormal.png", "T_MT_RowNormal"),
    ("T_MT_RowSelected.png", "T_MT_RowSelected"),
)


tasks = []
for source_name, asset_name in PARTS:
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing monster-tab source image: {source}")
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for _, asset_name in PARTS:
    asset_path = f"{DESTINATION}/{asset_name}"
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Monster-tab texture import failed: {asset_path}")
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    texture.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save monster-tab texture: {asset_path}")

unreal.log(f"RD_MONSTER_TAB_IMPORT success count={len(PARTS)} destination={DESTINATION}")
