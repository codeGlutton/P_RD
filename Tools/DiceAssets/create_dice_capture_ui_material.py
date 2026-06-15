import unreal


PACKAGE_PATH = "/Game/BP/UI/Dice"
MATERIAL_NAME = "M_DiceCaptureUI"


def main():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_path = f"{PACKAGE_PATH}/{MATERIAL_NAME}"
    material = unreal.load_asset(asset_path)
    if material is None:
        material = asset_tools.create_asset(
            MATERIAL_NAME,
            PACKAGE_PATH,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -520,
        -40,
    )
    texture_sample.set_editor_property("parameter_name", "DiceCaptureTexture")
    texture_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)

    one_minus = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionOneMinus,
        -220,
        120,
    )

    unreal.MaterialEditingLibrary.connect_material_expressions(texture_sample, "A", one_minus, "")
    unreal.MaterialEditingLibrary.connect_material_property(texture_sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(one_minus, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log(f"Created/updated {asset_path}")


if __name__ == "__main__":
    main()
