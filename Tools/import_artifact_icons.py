"""Import reviewed art to SVN and bind the production artifact data assets."""
from pathlib import Path
import json
import unreal

unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
source = Path(unreal.Paths.project_dir()) / 'SourceArt/UI/ArtifactIcons'
manifest = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))
destination = '/Game/SVN/OutSideAsset/AICreation/UI/Artifacts'
report = []
for row in manifest:
    filename = source / (row['icon'] + '_v1.png')
    data_path = '/Game/BP/DataAsset/Artifact/DA_Artifact_' + row['id']
    data = unreal.load_asset(data_path)
    if not filename.is_file() or not isinstance(data, unreal.StaticArtifactData):
        raise RuntimeError('Missing source or artifact: ' + row['id'])
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = destination
    task.destination_name = 'T_Artifact_' + row['icon']
    task.automated = True
    task.factory = unreal.TextureFactory()
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(destination + '/' + task.destination_name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Import failed: ' + row['icon'])
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('max_texture_size', 512)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError('Texture save failed: ' + row['icon'])
    data.set_editor_property('mIcon', texture)
    if not unreal.EditorAssetLibrary.save_loaded_asset(data):
        raise RuntimeError('Artifact save failed: ' + row['id'])
    report.append({'artifact': data.get_path_name(), 'icon': texture.get_path_name()})
    unreal.log('ARTIFACT_ICON_MAPPED ' + row['id'])
(Path(unreal.Paths.project_saved_dir()) / 'ArtifactIconMapping.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
