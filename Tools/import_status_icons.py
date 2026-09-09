"""Import the reviewed status PNG sources as UI textures; run with Unreal Python."""
from pathlib import Path
import unreal

# The legacy texture factory supports headless commandlets without Slate.
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
source = Path(unreal.Paths.project_dir()) / 'SourceArt/UI/StatusIcons'
names = ['Strength', 'Dexterity', 'Acumeny', 'Haste', 'Exhaustion', 'Slow', 'Frail', 'Root']
for name in names:
    filename = source / (name + '_v1.png')
    if not filename.is_file():
        raise RuntimeError('Missing source: ' + str(filename))
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = '/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons'
    task.destination_name = 'T_Status_' + name
    task.automated = True
    task.factory = unreal.TextureFactory()
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(task.destination_path + '/' + task.destination_name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Texture import failed: ' + name)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('max_texture_size', 512)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError('Save failed: ' + name)
    unreal.log('STATUS_ICON_IMPORTED ' + name)
