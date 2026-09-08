"""UE editor migration for the three vendored masked Effekseer materials.

ES3.1 masked materials cannot sample scene depth. Preserve the desktop soft
particle branch and all translucent materials; use a neutral fade on mobile
only in the masked variants. The generated function and materials belong in Git.
"""
import unreal


LIB = unreal.MaterialEditingLibrary
SOURCE = '/Effekseer/MaterialFunctions/EfkColorMatProcess'
TARGET = SOURCE + 'Masked'
MARKER = 'Masked ES3.1: scene-depth fade disabled'


def expressions(owner):
    return [item for item in unreal.ObjectIterator(unreal.MaterialExpression) if item.get_outer() == owner]


def migrate():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
        ['/Effekseer/MaterialFunctions', '/Effekseer/Materials'], True)
    function = unreal.load_asset(TARGET) if unreal.EditorAssetLibrary.does_asset_exist(TARGET) else unreal.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET)
    if not isinstance(function, unreal.MaterialFunction):
        raise RuntimeError('Could not create the masked color function')
    nodes = expressions(function)
    if not any(item.get_editor_property('desc') == MARKER for item in nodes):
        soft = next(item for item in nodes if isinstance(item, unreal.MaterialExpressionMaterialFunctionCall)
                    and item.get_editor_property('material_function').get_name() == 'EfkSoftParticle')
        multiply = next(item for item in nodes if item.get_name() == 'MaterialExpressionMultiply_0')
        level = LIB.create_material_expression_in_function(function, unreal.MaterialExpressionFeatureLevelSwitch, 600, 400)
        level.set_editor_property('desc', MARKER)
        neutral = LIB.create_material_expression_in_function(function, unreal.MaterialExpressionConstant, 400, 600)
        neutral.set_editor_property('r', 1.0)
        if not all((LIB.connect_material_expressions(soft, 'Result', level, 'Default'),
                    LIB.connect_material_expressions(neutral, '', level, 'ES3_1'),
                    LIB.connect_material_expressions(level, '', multiply, 'B'))):
            raise RuntimeError('Masked soft-particle graph connections failed')
        LIB.update_material_function(function)
        if not unreal.EditorAssetLibrary.save_loaded_asset(function):
            raise RuntimeError('Could not save the masked color function')
    for name in ('M_Opaque', 'M_Opaque_DD', 'M_Lighting'):
        material = unreal.load_asset('/Effekseer/Materials/' + name)
        if material.get_editor_property('blend_mode') != unreal.BlendMode.BLEND_MASKED:
            raise RuntimeError('Review shader migration after changing blend mode: ' + name)
        calls = [item for item in expressions(material) if isinstance(item, unreal.MaterialExpressionMaterialFunctionCall)
                 and item.get_editor_property('material_function')
                 and item.get_editor_property('material_function').get_name() in ('EfkColorMatProcess', 'EfkColorMatProcessMasked')]
        if len(calls) != 1:
            raise RuntimeError('Expected one color processing function: ' + name)
        calls[0].set_editor_property('material_function', function)
        LIB.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError('Could not save migrated masked material: ' + name)
    unreal.log('Masked Effekseer ES3.1 migration saved. Verify Vulkan ES3.1 and OpenGL ES3.1 cooking.')


migrate()
