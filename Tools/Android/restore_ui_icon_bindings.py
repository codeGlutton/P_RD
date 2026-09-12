"""Restore reviewed icon references to existing SVN textures without replacing gameplay data.

Run using UnrealEditor-Cmd -run=pythonscript -script=<this file>.
The associated manifest contains texture fields only. SVN must be updated first.
"""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
manifest = json.loads((root / 'SourceArt/UI/BindingRepair_20260909/manifest.json').read_text(encoding='utf-8'))
pending = []
for row in manifest['bindings']:
    if set(row) != {'asset', 'textures'}:
        raise RuntimeError('Only texture bindings may be restored: ' + row['asset'])
    asset = unreal.load_asset(row['asset'])
    if asset is None:
        raise RuntimeError('Missing data asset: ' + row['asset'])
    for field, path in row['textures'].items():
        if field not in ('mIcon', 'mPortrait'):
            raise RuntimeError('Unexpected property: ' + field)
        texture = unreal.load_asset(path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError('Missing texture: ' + path)
        pending.append((asset, field, texture))

changed = {}
for asset, field, texture in pending:
    before = asset.get_editor_property(field)
    if before == texture:
        continue
    asset.set_editor_property(field, texture)
    changed[asset.get_path_name()] = asset
for path, asset in changed.items():
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError('Could not save: ' + path)
report = {'bindings_checked': len(pending), 'assets_changed': list(changed)}
(root / 'Saved/UIIconRestoration.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('UI_ICON_RESTORATION_COMPLETE ' + json.dumps(report))
