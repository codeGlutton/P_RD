"""Run in Unreal Python. Import the silhouette and build texture-free UI light.

Original SVN color frames are read only. The mask removes their baked outer
light without reducing the resolution of the wood, metal or coin detail.
"""
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/UI/Reward/Separated'
LIB = unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
task = unreal.AssetImportTask()
task.filename = str(ROOT / 'SourceArt/UI/RewardChest/chest_silhouette_v1.png')
task.destination_path = DEST
task.destination_name = 'T_RewardChestSilhouette'
task.automated = True
task.factory = unreal.TextureFactory()
task.replace_existing = True
task.save = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mask = unreal.load_asset(DEST + '/T_RewardChestSilhouette')
assert isinstance(mask, unreal.Texture2D)
# NPOT atlases cannot always use block compression on Android. G8 keeps this
# single-channel mask at 11 MB instead of allocating a second 45 MB RGBA atlas.
mask.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_GRAYSCALE)
mask.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
mask.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
mask.set_editor_property('srgb', False)
mask.set_editor_property('never_stream', True)
mask.set_editor_property('max_texture_size', 0)
assert unreal.EditorAssetLibrary.save_loaded_asset(mask)


def build(name, filename, textures, additive=False):
    material = unreal.load_asset(DEST + '/' + name)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE
                                     if additive else unreal.BlendMode.BLEND_TRANSLUCENT)
        nodes = {}
        nodes['UV'] = LIB.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -600, 0)
        nodes['Frame'] = LIB.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 160)
        nodes['Frame'].set_editor_property('parameter_name', 'Frame')
        for index, (param, texture) in enumerate(textures.items()):
            node = LIB.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, -600, 320 + index * 160)
            node.set_editor_property('parameter_name', param)
            node.set_editor_property('texture', texture)
            nodes[param] = node
        shader = LIB.create_material_expression(material, unreal.MaterialExpressionCustom, -200, 0)
        shader.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
        inputs = []
        for param in nodes:
            item = unreal.CustomInput()
            item.set_editor_property('input_name', param)
            inputs.append(item)
        shader.set_editor_property('inputs', inputs)
        for param, node in nodes.items():
            assert LIB.connect_material_expressions(node, '', shader, param)
        alpha = LIB.create_material_expression(material, unreal.MaterialExpressionComponentMask, 0, 180)
        for param, value in [('r', False), ('g', False), ('b', False), ('a', True)]:
            alpha.set_editor_property(param, value)
        assert LIB.connect_material_expressions(shader, '', alpha, '')
        assert LIB.connect_material_property(shader, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        assert LIB.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    else:
        # Do not delete constructor-rooted expressions during a rebuild.
        shader = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        assert isinstance(shader, unreal.MaterialExpressionCustom)
    shader.set_editor_property('code', (ROOT / 'Tools/RewardChest' / filename).read_text(encoding='utf-8'))
    LIB.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)


build('M_RewardChestCutout', 'chest_cutout.ush', {
    'Atlas': unreal.load_asset('/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New/T_RCN_ChestTripleBurst_Atlas'),
    'Silhouette': mask,
})
build('M_RewardChestLight', 'chest_light.ush', {}, additive=True)
