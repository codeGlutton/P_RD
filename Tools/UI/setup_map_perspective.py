"""평평한 지도 텍스처 임포트 + 화면 고정 원근(RetainerBox용) UI 머티리얼 생성.

지도 원근은 이제 이미지에 굽지 않고 M_MapPerspective 머티리얼이 화면에서 건다.
C++ FrontendMapWidget이 스크롤 레이어를 RetainerBox로 감싸고 이 머티리얼을 쓴다.
"""

import unreal

DESTINATION_PATH = "/Game/UI/Art/RunFlow"
# 지도 텍스처 원본은 SVN(아트 정본)에 있다. 여기서는 임포트하지 않고 참조만 한다.
TEXTURE_PATH = "/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_StageMap_Scroll_Flat"
MATERIAL_NAME = "M_MapPerspective"

# C++ FrontendMapWidget::MapPerspectiveTopWidth 와 같은 값이어야 한다.
DEFAULT_TOP_WIDTH = 0.85

PERSPECTIVE_HLSL = """
float W = clamp(TopWidth, 0.05, 1.0);
float v = saturate(uv.y);
float t = v / (W + (1.0 - W) * v);
float D = (W - 1.0) * t + 1.0;
float s = (uv.x * D - (0.5 - 0.5 * W) * (1.0 - t)) / W;
float edge = max(fwidth(s) * 0.75, 0.0005);
float mask = smoothstep(0.0, edge, s) * smoothstep(0.0, edge, 1.0 - s);
float4 c = Texture2DSample(Tex, TexSampler, float2(saturate(s), t));
return float4(c.rgb, mask);
"""


def create_perspective_material(default_texture: unreal.Texture2D) -> None:
    asset_path = f"{DESTINATION_PATH}/{MATERIAL_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(
        MATERIAL_NAME, DESTINATION_PATH, unreal.Material, unreal.MaterialFactoryNew()
    )
    if material is None:
        raise RuntimeError(f"Could not create material: {asset_path}")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    mel = unreal.MaterialEditingLibrary

    texcoord = mel.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -900, -100
    )

    tex_param = mel.create_material_expression(
        material, unreal.MaterialExpressionTextureObjectParameter, -900, 100
    )
    tex_param.set_editor_property("parameter_name", "Texture")
    tex_param.set_editor_property("texture", default_texture)

    width_param = mel.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, 300
    )
    width_param.set_editor_property("parameter_name", "TopWidth")
    width_param.set_editor_property("default_value", DEFAULT_TOP_WIDTH)

    custom = mel.create_material_expression(
        material, unreal.MaterialExpressionCustom, -500, 0
    )
    custom.set_editor_property("code", PERSPECTIVE_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    custom.set_editor_property("description", "ScreenFixedPerspective")
    inputs = []
    for name in ("uv", "Tex", "TopWidth"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        inputs.append(entry)
    custom.set_editor_property("inputs", inputs)

    if not mel.connect_material_expressions(texcoord, "", custom, "uv"):
        raise RuntimeError("connect texcoord -> custom.uv failed")
    if not mel.connect_material_expressions(tex_param, "", custom, "Tex"):
        raise RuntimeError("connect texture -> custom.Tex failed")
    if not mel.connect_material_expressions(width_param, "", custom, "TopWidth"):
        raise RuntimeError("connect width -> custom.TopWidth failed")

    mask_rgb = mel.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, -200, -80
    )
    mask_rgb.set_editor_property("r", True)
    mask_rgb.set_editor_property("g", True)
    mask_rgb.set_editor_property("b", True)
    mask_rgb.set_editor_property("a", False)

    mask_a = mel.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, -200, 120
    )
    mask_a.set_editor_property("r", False)
    mask_a.set_editor_property("g", False)
    mask_a.set_editor_property("b", False)
    mask_a.set_editor_property("a", True)

    if not mel.connect_material_expressions(custom, "", mask_rgb, ""):
        raise RuntimeError("connect custom -> mask_rgb failed")
    if not mel.connect_material_expressions(custom, "", mask_a, ""):
        raise RuntimeError("connect custom -> mask_a failed")

    if not mel.connect_material_property(
        mask_rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("connect mask_rgb -> EmissiveColor failed")
    if not mel.connect_material_property(mask_a, "", unreal.MaterialProperty.MP_OPACITY):
        raise RuntimeError("connect mask_a -> Opacity failed")

    mel.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save material: {asset_path}")

    unreal.log(f"RD_MAP_PERSPECTIVE material={asset_path}")


def main() -> None:
    texture = unreal.load_asset(TEXTURE_PATH)
    if texture is None:
        raise RuntimeError(f"지도 텍스처를 찾지 못했다: {TEXTURE_PATH}")
    create_perspective_material(texture)


main()
