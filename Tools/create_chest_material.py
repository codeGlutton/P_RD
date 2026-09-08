import unreal

path = '/Game/UI/Reward/M_RewardChestSoftAtlas'
material = unreal.load_asset(path)
if material is not None:
    # Constructor-loaded assets are rooted; do not delete their expressions.
    shader = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert isinstance(shader, unreal.MaterialExpressionCustom), 'Unexpected existing material graph'
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_RewardChestSoftAtlas', '/Game/UI/Reward', unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    lib = unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(material)
    uv = lib.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -600, 0)
    frame = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 150)
    frame.set_editor_property('parameter_name', 'Frame')
    atlas = lib.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, -600, 300)
    atlas.set_editor_property('parameter_name', 'Atlas')
    atlas.set_editor_property('texture', unreal.load_asset('/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New/T_RCN_ChestTripleBurst_Atlas'))
    shader = lib.create_material_expression(material, unreal.MaterialExpressionCustom, -250, 0)
    shader.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs=[]
    for name in ['UV', 'Frame', 'Atlas']:
        item=unreal.CustomInput()
        item.set_editor_property('input_name', name)
        inputs.append(item)
    shader.set_editor_property('inputs', inputs)
    for node,name in [(uv,'UV'), (frame,'Frame'), (atlas,'Atlas')]:
        assert lib.connect_material_expressions(node, '', shader, name), name
    alpha = lib.create_material_expression(material, unreal.MaterialExpressionComponentMask, 0, 180)
    for name,value in [('r',False),('g',False),('b',False),('a',True)]:
        alpha.set_editor_property(name,value)
    assert lib.connect_material_expressions(shader, '', alpha, '')
    assert lib.connect_material_property(shader, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert lib.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)

# Keep the source chest at its original on-screen size, inside a larger canvas.
# A radial halo fills the space outside the atlas instead of exposing its box.
shader.set_editor_property('code', '''
float2 sourceUV = (UV - 0.5) * 1.5 + 0.5;
float2 cellUV = (clamp(sourceUV, float2(0.5/682.0, 0.5/455.0),
    float2(1.0-0.5/682.0, 1.0-0.5/455.0)) + Frame.xy) / 6.0;
float4 source = Texture2DSample(Atlas, AtlasSampler, cellUV);
float radius = length((sourceUV - 0.5) * 2.0);
float sourceAlpha = source.a * (1.0 - smoothstep(0.72, 1.0, radius));
float frameIndex = Frame.x + Frame.y * 6.0;
float burst = smoothstep(3.0, 9.0, frameIndex)
    * (1.0 - 0.6 * smoothstep(20.0, 32.0, frameIndex));
float halo = 0.65 * burst * pow(saturate(1.0 - length((UV - 0.5) * 2.0)), 2.0);
float alpha = sourceAlpha + halo * (1.0 - sourceAlpha);
float3 rgb = (source.rgb * sourceAlpha
    + float3(1.0, 0.42, 0.055) * halo * (1.0 - sourceAlpha)) / max(alpha, 0.0001);
return float4(rgb, alpha);
''')
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
