from pathlib import Path
import unreal

root = Path(unreal.Paths.project_saved_dir()) / 'StatusIconReview'
root.mkdir(parents=True, exist_ok=True)
for name in ['Weakness', 'Poison', 'Agility', 'Fortification']:
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset('/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_' + name)
    task.filename = str(root / (name + '.png'))
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.TextureExporterPNG()
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(name)
