import unreal
from pathlib import Path
source = Path(unreal.Paths.project_dir()) / 'SourceArt/ThirdParty/EffekseerHoly/Effects'
dest = '/Game/UI/RewardConcept03New/Effekseer'
tasks=[]
for name in ['tx_ring01_256','tx_shockring01_256','tx_star01_256']:
    if unreal.EditorAssetLibrary.does_asset_exist(dest+'/Textures/'+name): continue
    t=unreal.AssetImportTask()
    t.filename=str(source/'Textures'/(name+'.png'))
    t.destination_path=dest+'/Textures'
    t.automated=True
    t.save=True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
t=unreal.AssetImportTask()
t.filename=str(source/'chest_radiance.efkefc')
t.destination_path=dest
t.automated=True
t.replace_existing=True
t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
assert unreal.load_asset(dest+'/chest_radiance')
unreal.log('CHEST_RADIANCE_IMPORTED')
