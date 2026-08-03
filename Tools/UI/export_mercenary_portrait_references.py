"""Export the six existing mercenary portrait textures as PNG references."""

from pathlib import Path

import unreal


OUTPUT_DIR = Path(unreal.Paths.project_dir()) / "SourceArt" / "UI" / "Marchbound" / "MercenaryReferences"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

CHARACTERS = (
    "Knight",
    "Mage",
    "Ranger",
    "Rogue",
    "RogueHooded",
    "Barbarian",
    "Druid",
)

for character in CHARACTERS:
    for kind in ("HeadV2", "ActionV3"):
        asset_path = (
            "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/"
            f"KK_Face_{character}_{kind}"
        )
        asset = unreal.load_asset(asset_path)
        if asset is None:
            raise RuntimeError(f"Missing reference texture: {asset_path}")

        task = unreal.AssetExportTask()
        task.object = asset
        task.filename = str(OUTPUT_DIR / f"{character}_{kind}.png")
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        if not unreal.Exporter.run_asset_export_task(task):
            raise RuntimeError(f"Failed to export {asset_path}")

unreal.log(f"RD_MB_HIRE_REFERENCE_EXPORT success output={OUTPUT_DIR}")
