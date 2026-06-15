import unreal


PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = PROJECT_ROOT + "Content/SourceArt/Dice/Generated"
DESTINATION_PATH = "/Game/BP/UI/Dice"
MATERIAL_PATH = DESTINATION_PATH + "/M_DiceFace"


def import_assets() -> None:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []

    texture_task = unreal.AssetImportTask()
    texture_task.filename = f"{SOURCE_DIR}/T_DiceFace_Base.png"
    texture_task.destination_path = DESTINATION_PATH
    texture_task.destination_name = "T_DiceFace_Base"
    texture_task.automated = True
    texture_task.replace_existing = True
    texture_task.save = True
    tasks.append(texture_task)

    asset_tools.import_asset_tasks(tasks)

    texture = unreal.EditorAssetLibrary.load_asset(f"{DESTINATION_PATH}/T_DiceFace_Base")
    if texture is not None:
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)


def create_or_update_material() -> None:
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
    if material is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            "M_DiceFace",
            DESTINATION_PATH,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    if material is None:
        raise RuntimeError("M_DiceFace material could not be created")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    texture_param = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -320,
        0,
    )
    texture_param.set_editor_property("parameter_name", "FaceTexture")

    default_texture = unreal.EditorAssetLibrary.load_asset(f"{DESTINATION_PATH}/T_DiceFace_Base")
    if default_texture is not None:
        texture_param.set_editor_property("texture", default_texture)

    unreal.MaterialEditingLibrary.connect_material_property(
        texture_param,
        "RGB",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def main() -> None:
    import_assets()
    create_or_update_material()


if __name__ == "__main__":
    main()
