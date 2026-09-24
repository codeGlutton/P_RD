"""Import beta 15 icons and bind the eight new production data assets."""
from pathlib import Path
import json
import unreal

unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
source = Path(unreal.Paths.project_dir()) / 'SourceArt/UI/ReleaseIcons'
destination = '/Game/BP/ReleaseIcons'
targets = [
    ('BrokenClock', '/Game/BP/DataAsset/Artifact/DA_Artifact_A046_BrokenClock'),
    ('MagicHat', '/Game/BP/DataAsset/Artifact/DA_Artifact_A047_MagicHat'),
    ('Inkpot', '/Game/BP/DataAsset/Artifact/DA_Artifact_A048_Inkpot'),
    ('HourglassTurn', '/Game/BP/DataAsset/Skill/Common/DA_Common_Spell_Epic_HourglassTurn'),
    ('BlessingOfTime', '/Game/BP/DataAsset/Skill/Mercenary/Druid/DA_Druid_Spell_Epic_BlessingOfTime'),
    ('TickTock', '/Game/BP/DataAsset/Skill/Mercenary/Mage/DA_Mage_Spell_Epic_TickTock'),
    ('TimeAcceleration', '/Game/BP/DataAsset/Skill/Mercenary/Mage/DA_Mage_Spell_Rare_TimeAcceleration'),
    ('Cheat', '/Game/BP/DataAsset/Skill/Mercenary/Rogue/DA_Rogue_Spell_Epic_Cheat'),
]
report = []
for name, asset_path in targets:
    filename = source / (name + '.png')
    data = unreal.load_asset(asset_path)
    if not filename.is_file() or data is None:
        raise RuntimeError('Missing source image or data asset: ' + name)
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = destination
    task.destination_name = 'T_Beta15_' + name
    task.automated = True
    task.factory = unreal.TextureFactory()
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(destination + '/' + task.destination_name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Texture import failed: ' + name)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('max_texture_size', 512)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError('Texture save failed: ' + name)
    data.set_editor_property('mIcon', texture)
    if not unreal.EditorAssetLibrary.save_loaded_asset(data):
        raise RuntimeError('Data asset save failed: ' + name)
    report.append({'asset': data.get_path_name(), 'icon': texture.get_path_name()})
    unreal.log('RELEASE_ICON_MAPPED ' + name)
report_path = Path(unreal.Paths.project_saved_dir()) / 'ReleaseIconBindings.json'
report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('RELEASE_ICON_IMPORT_COMPLETE count=%d report=%s' % (len(report), report_path))
