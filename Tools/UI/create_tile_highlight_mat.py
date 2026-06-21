# @brief 타일 하이라이트 표시용 머티리얼 생성.
# TileMap.cpp가 SetCustomDataValue로 쓴 PerInstanceCustomData[0..3](RGBA, 프리멀티)를 읽어
# Emissive = highlightRGB + baseGray*(1-A) 로 출력한다(Unlit).
import unreal

FOLDER = "/Game/BP/Materials"
NAME = "M_TileHighlight"
PATH = FOLDER + "/" + NAME

tools = unreal.AssetToolsHelpers.get_asset_tools()
if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    unreal.EditorAssetLibrary.delete_asset(PATH)

mat = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("used_with_instanced_static_meshes", True)

MEL = unreal.MaterialEditingLibrary

def pcd(index, y):
    n = MEL.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, -900, y)
    n.set_editor_property("data_index", index)
    return n

p0 = pcd(0, -400)
p1 = pcd(1, -300)
p2 = pcd(2, -200)
p3 = pcd(3, 200)

# highlightRGB = append(append(p0,p1), p2)
app1 = MEL.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -650, -350)
MEL.connect_material_expressions(p0, "", app1, "A")
MEL.connect_material_expressions(p1, "", app1, "B")
appRGB = MEL.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -500, -300)
MEL.connect_material_expressions(app1, "", appRGB, "A")
MEL.connect_material_expressions(p2, "", appRGB, "B")

# base gray
base = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -650, 100)
base.set_editor_property("constant", unreal.LinearColor(0.12, 0.12, 0.12, 1.0))

# (1 - A) = Subtract(1.0, p3)
one = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -650, 300)
one.set_editor_property("r", 1.0)
sub = MEL.create_material_expression(mat, unreal.MaterialExpressionSubtract, -500, 250)
MEL.connect_material_expressions(one, "", sub, "A")
MEL.connect_material_expressions(p3, "", sub, "B")

# base * (1-A)
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 150)
MEL.connect_material_expressions(base, "", mul, "A")
MEL.connect_material_expressions(sub, "", mul, "B")

# final = highlightRGB + base*(1-A)
add = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -150, 0)
MEL.connect_material_expressions(appRGB, "", add, "A")
MEL.connect_material_expressions(mul, "", add, "B")

MEL.connect_material_property(add, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(PATH)
print("[TileMat] DONE created " + PATH)
