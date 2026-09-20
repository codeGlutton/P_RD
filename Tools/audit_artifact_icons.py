from pathlib import Path
import json
import unreal

rows = []
for path in unreal.EditorAssetLibrary.list_assets('/Game/BP/DataAsset/Artifact', recursive=True):
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.StaticArtifactData):
        continue
    icon = asset.get_editor_property('mIcon')
    rows.append({'path': path, 'name': str(asset.get_editor_property('mName')),
                 'icon': icon.get_path_name() if icon else None,
                 'modifiers': str(asset.get_editor_property('mStatModifiers')),
                 'passives': str(asset.get_editor_property('mStaticPassiveData'))})
output = Path(unreal.Paths.project_saved_dir()) / 'ArtifactIconAudit.json'
output.write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding='utf-8')
