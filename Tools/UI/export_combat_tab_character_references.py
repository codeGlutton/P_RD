"""Export the exact project head portraits used as combat-tab art references."""

from pathlib import Path

import unreal


OUTPUT_DIR = (
    Path(unreal.Paths.project_dir())
    / "SourceArt"
    / "UI"
    / "Marchbound"
    / "Combat"
    / "References"
)
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

EXPORTS = (
    (
        "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Knight_HeadV2",
        "Knight_HeadV2_ProjectReference.png",
    ),
    (
        "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_HeadV2",
        "Werewolf_HeadV2_ProjectReference.png",
    ),
)

for asset_path, output_name in EXPORTS:
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Missing reference texture: {asset_path}")
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = str(OUTPUT_DIR / output_name)
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"Failed to export {asset_path}")

unreal.log(f"RD_COMBAT_TAB_REFERENCE_EXPORT success output={OUTPUT_DIR}")
