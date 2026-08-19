import os

import unreal


output_directory = os.path.join(
    unreal.Paths.project_saved_dir(), "UI", "CombatDetails", "SquareCellRefs"
)
os.makedirs(output_directory, exist_ok=True)

for asset_path in (
    "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Disabled",
    "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Selected",
):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Missing texture: {asset_path}")
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = os.path.join(output_directory, asset.get_name() + ".png")
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.TextureExporterPNG()
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"Failed to export: {asset_path}")

unreal.log(f"RD_SQUARE_CELL_EXPORT_DONE path={output_directory}")
unreal.SystemLibrary.quit_editor()
