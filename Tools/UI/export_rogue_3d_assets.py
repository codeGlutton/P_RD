"""Export the two source KayKit rogue skeletal meshes for direct 3D inspection."""

from pathlib import Path

import unreal


OUTPUT_DIR = Path(unreal.Paths.project_dir()) / "work" / "Rogue3DInspection" / "exports"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

ASSETS = {
    "Rogue": (
        "/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/Charaters/"
        "Rogue/SkeletalMeshes/SK_Rogue"
    ),
    "Rogue_Hooded": (
        "/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/Charaters/"
        "Rogue_Hooded/SkeletalMeshes/SK_Rogue_Hooded"
    ),
}

for output_name, asset_path in ASSETS.items():
    asset = unreal.load_asset(asset_path)
    if not isinstance(asset, unreal.SkeletalMesh):
        raise RuntimeError(f"Missing skeletal mesh: {asset_path}")

    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = str(OUTPUT_DIR / f"SK_{output_name}.fbx")
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.write_empty_files = False
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"Failed to export skeletal mesh: {asset_path}")

    unreal.log(
        f"RD_ROGUE_3D_EXPORT name={output_name} asset={asset_path} "
        f"output={task.filename}"
    )

