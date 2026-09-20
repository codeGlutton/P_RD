"""Import the reviewed monster variants and update only their UI texture bindings.

Run with Unreal's PythonScript commandlet. PNG sources are preserved in SVN
SourceArt/UI/MonsterPortraits_20260916; copy that folder into project SourceArt/UI
before running. Gameplay, skills, meshes and cut-in bindings are not modified.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(unreal.Paths.project_dir())
SOURCE = ROOT / 'SourceArt/UI/MonsterPortraits_20260916'
DEST = '/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916'
BINDINGS = {
    'SkeletonBird': '/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_SkeletonBirdUnit',
    'RedSpider': '/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_Red_SpiderUnit',
    'SlimeExplosion': '/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_Slime_ExplosionUnit',
}

unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
changes = []
for name, asset_path in BINDINGS.items():
    source = SOURCE / (name + '_v1.png')
    assert source.is_file(), str(source)
    texture_path = DEST + '/T_Portrait_' + name + '_v1'
    texture = unreal.load_asset(texture_path) if unreal.EditorAssetLibrary.does_asset_exist(texture_path) else None
    if texture is None:
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = DEST
        task.destination_name = 'T_Portrait_' + name + '_v1'
        task.factory = unreal.TextureFactory()
        task.automated = True
        task.replace_existing = False
        task.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(texture_path)
    assert isinstance(texture, unreal.Texture2D), texture_path
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('max_texture_size', 512)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    asset = unreal.load_asset(asset_path)
    assert isinstance(asset, unreal.StaticEnemyUnitSpawnData), asset_path
    row = {'asset': asset_path, 'texture': texture_path}
    for field in ('mIcon', 'mPortrait'):
        old = asset.get_editor_property(field)
        row[field + '_before'] = old.get_path_name() if old else None
        asset.set_editor_property(field, texture)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    changes.append(row)

output = Path(unreal.Paths.project_saved_dir()) / 'MonsterPortraitImport.json'
output.write_text(json.dumps(changes, indent=2), encoding='utf-8')
unreal.log('MONSTER_PORTRAIT_IMPORT_COMPLETE count=%d' % len(changes))
