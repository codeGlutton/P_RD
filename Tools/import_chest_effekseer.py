import unreal
from pathlib import Path

source = Path(unreal.Paths.project_dir()) / 'SourceArt/ThirdParty/EffekseerHoly/Effects'
dest = '/Game/UI/RewardConcept03New/Effekseer'
names = ['tx_glow01_128', 'tx_glow02_128', 'tx_glow03_128', 'tx_prism_aura01', 'tx_trail02_256_90']
tasks = []
for name in names:
    task = unreal.AssetImportTask()
    task.filename = str(source / 'Textures' / (name + '.png'))
    task.destination_path = dest + '/Textures'
    task.automated = True
    task.save = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
task = unreal.AssetImportTask()
task.filename = str(source / 'ef_holy01.efkefc')
task.destination_path = dest
task.automated = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assert unreal.EditorAssetLibrary.does_asset_exist(dest + '/ef_holy01')

mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ChestLiveParticles', dest, unreal.Material, unreal.MaterialFactoryNew())
if not mat:
    mat = unreal.load_asset(dest + '/M_ChestLiveParticles')
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
lib = unreal.MaterialEditingLibrary
lib.delete_all_material_expressions(mat)
sample = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D)
sample.set_editor_property('parameter_name', 'Particles')
sample.set_editor_property('texture', unreal.load_asset(dest + '/Textures/tx_glow01_128'))
lib.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
one = lib.create_material_expression(mat, unreal.MaterialExpressionConstant)
one.set_editor_property('r', 1.0)
lib.connect_material_property(one, '', unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(dest + '/M_ChestLiveParticles')
unreal.log('CHEST_LIVE_VFX_IMPORTED')
