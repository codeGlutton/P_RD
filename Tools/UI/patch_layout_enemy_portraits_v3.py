"""Force every baked layout's enemy portrait brush to the V3 eagle action."""

import unreal


ROOT = "/Game/UI/CombatLayouts"
TEXTURE_PATH = (
    "/Game/SVN/OutSideAsset/UI/KayKit/KK_Face_Enemy_Eagle_ActionV3"
)

texture = unreal.EditorAssetLibrary.load_asset(TEXTURE_PATH)
if texture is None:
    raise RuntimeError("monster action texture missing: " + TEXTURE_PATH)

patched = 0
for object_path in unreal.EditorAssetLibrary.list_assets(ROOT, False, False):
    package = str(object_path).split(".", 1)[0]
    if "/WBP_CombatLayout_" not in package:
        continue
    blueprint = unreal.EditorAssetLibrary.load_asset(package)
    if blueprint is None:
        continue
    widget = unreal.MCPythonHelper.umg_find_widget(blueprint, "EnemyPortrait")
    if widget is None:
        continue
    brush = widget.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    widget.set_editor_property("brush", brush)
    blueprint.modify(True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError("layout did not save: " + package)
    patched += 1

unreal.log(
    "[KayKit Monster Actions V3] patched {} layout enemy portraits".format(
        patched
    )
)
