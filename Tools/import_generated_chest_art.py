"""Run in Unreal's Python commandlet after building P_RDEditor.

Imports the checked-in ImageGen sources; leaves shared SVN textures untouched.
"""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve() / 'SourceArt/UI/GeneratedChest'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for filename, name, mode in [
    ('gold_dome_v2.png', 'T_ChestGoldDome_V2', 'light'),
    ('chest_clean_atlas_v2.png', 'T_ChestCleanAtlas_V2', 'atlas'),
]:
    source = root / filename
    if not source.is_file():
        raise FileNotFoundError(source)
    processed = Path(unreal.Paths.project_saved_dir()).resolve() / (name + '.png')
    previous = processed.stat().st_mtime_ns if processed.exists() else 0
    unreal.SystemLibrary.execute_console_command(
        world, f'RD.Editor.ImportChestArt "{source.as_posix()}" {name} {mode}')
    if not unreal.EditorAssetLibrary.does_asset_exist(
        '/Game/UI/RewardConcept03New/Generated/' + name):
        raise RuntimeError('Chest art import failed: ' + name)
    if not processed.exists() or processed.stat().st_mtime_ns <= previous:
        raise RuntimeError('Chest art importer did not write its output: ' + name)
