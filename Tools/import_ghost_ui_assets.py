"""Import the Ghost's portrait/cut-in and bind its three UI fields.

Run with UnrealEditor-Cmd PythonScript after mounting SVN under Content/SVN.
The generated PNG originals live in SVN/SourceArt/UI/Ghost_20260919.
"""

from pathlib import Path
import json
import unreal


ROOT = Path(unreal.Paths.project_dir())
SOURCE = ROOT / 'Content/SVN/SourceArt/UI/Ghost_20260919'
PORTRAIT_PATH = '/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260919/T_Portrait_Ghost_v1'
CUTIN_PATH = '/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Ghost_v1'
GHOST_DA_PATH = '/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_GhostUnit'


def import_texture(filename, asset_path, max_size):
    source = SOURCE / filename
    assert source.is_file(), source
    assert not unreal.EditorAssetLibrary.does_asset_exist(asset_path), asset_path

    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path, task.destination_name = asset_path.rsplit('/', 1)
    task.factory = unreal.TextureFactory()
    task.automated = True
    task.replace_existing = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(asset_path)
    assert isinstance(texture, unreal.Texture2D), asset_path
    for prop, value in {
        'compression_settings': unreal.TextureCompressionSettings.TC_EDITOR_ICON,
        'lod_group': unreal.TextureGroup.TEXTUREGROUP_UI,
        'mip_gen_settings': unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        'srgb': True,
        'max_texture_size': max_size,
        'never_stream': True,
        'address_x': unreal.TextureAddress.TA_CLAMP,
        'address_y': unreal.TextureAddress.TA_CLAMP,
    }.items():
        texture.set_editor_property(prop, value)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture), asset_path
    return texture


unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
ghost = unreal.load_asset(GHOST_DA_PATH)
assert isinstance(ghost, unreal.StaticEnemyUnitSpawnData), GHOST_DA_PATH

previous = {}
for field in ('mIcon', 'mPortrait', 'mShortCut'):
    asset = ghost.get_editor_property(field)
    previous[field] = asset.get_path_name() if asset else None
    assert previous[field] and 'Ghoul' in previous[field], (field, previous[field])

portrait = import_texture('Ghost_Portrait_v1.png', PORTRAIT_PATH, 512)
cutin = import_texture('Ghost_CutIn_v1.png', CUTIN_PATH, 2048)

ghost.set_editor_property('mIcon', portrait)
ghost.set_editor_property('mPortrait', portrait)
ghost.set_editor_property('mShortCut', cutin)
assert unreal.EditorAssetLibrary.save_loaded_asset(ghost), GHOST_DA_PATH

reloaded = unreal.load_asset(GHOST_DA_PATH)
expected = {'mIcon': PORTRAIT_PATH, 'mPortrait': PORTRAIT_PATH, 'mShortCut': CUTIN_PATH}
for field, path in expected.items():
    actual = reloaded.get_editor_property(field)
    assert actual and actual.get_path_name().split('.')[0] == path, (field, actual)

report = {'asset': GHOST_DA_PATH, 'previous': previous, 'updated': expected}
(Path(unreal.Paths.project_saved_dir()) / 'GhostUiImport.json').write_text(
    json.dumps(report, indent=2), encoding='utf-8')
unreal.log('GHOST_UI_IMPORT_PASS')
