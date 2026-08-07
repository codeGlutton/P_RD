"""Retune the committed monster-tab WBP without rebuilding its WidgetTree.

This is safe to run against the PR #474 asset: it preserves widget names and
runtime bindings, and only changes authored CanvasPanel bounds and font sizes.
"""

import unreal


ASSET_PATH = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"
OBJECT_PREFIX = ASSET_PATH + ".WBP_MonsterTab_Marchbound:WidgetTree."


def widget(name, expected_type=None):
    result = unreal.load_object(None, OBJECT_PREFIX + name)
    if result is None:
        raise RuntimeError(f"Missing committed monster-tab widget: {name}")
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, "
            f"expected {expected_type.__name__}"
        )
    result.modify()
    return result


def canvas_bounds(target, position, size, z_order=None):
    slot = target.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{target.get_name()} must use CanvasPanelSlot")
    slot.modify()
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(*position))
    slot.set_size(unreal.Vector2D(*size))
    if z_order is not None:
        slot.set_z_order(z_order)


def font_size(name, size):
    target = widget(name, unreal.TextBlock)
    font = target.get_editor_property("font")
    font.set_editor_property("size", size)
    target.set_editor_property("font", font)


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if asset is None:
    raise RuntimeError(f"Missing monster-tab WBP: {ASSET_PATH}")
asset.modify()

# Header: the title stays centred over the roster/portrait pair. The back
# action moves into the detail sheet so it cannot overlap the combat option
# rail that remains visible behind this modal.
canvas_bounds(widget("MonsterTabTitleText"), (340.0, 69.0), (640.0, 104.0), 10)
font_size("MonsterTabTitleText", 62)
canvas_bounds(widget("MonsterBackArt"), (1650.0, 151.0), (190.0, 76.0), 30)
canvas_bounds(widget("MonsterBackText"), (1650.0, 157.0), (190.0, 60.0), 31)
font_size("MonsterBackText", 29)
canvas_bounds(widget("MonsterBackButton"), (1650.0, 151.0), (190.0, 76.0), 32)

# Roster: preserve the source row artwork's 2.79:1 aspect ratio and increase
# only the live portrait/name content. This avoids the vertical stretching in
# the previous mobile pass.
for index in range(3):
    canvas_bounds(widget(f"MonsterRow_{index}"), (42.0, 260.0 + 225.0 * index), (448.0, 160.0), 20)
    canvas_bounds(widget(f"MonsterRowNormal_{index}"), (0.0, 0.0), (448.0, 160.0), 0)
    canvas_bounds(widget(f"MonsterRowSelected_{index}"), (0.0, 0.0), (448.0, 160.0), 2)
    canvas_bounds(widget(f"MonsterRowPortrait_{index}"), (30.0, 15.0), (130.0, 130.0), 6)
    canvas_bounds(widget(f"MonsterRowName_{index}"), (168.0, 43.0), (246.0, 74.0), 8)
    font_size(f"MonsterRowName_{index}", 38)
    canvas_bounds(widget(f"MonsterRowButton_{index}"), (0.0, 0.0), (448.0, 160.0), 20)

# Hero read: give full-body art the whole centre sheet. Square/head-only
# fallback portraits still remain centred because the ScaleBox keeps ScaleToFit.
canvas_bounds(widget("MonsterDetailPortraitScale"), (530.0, 220.0), (530.0, 685.0), 15)
canvas_bounds(widget("MonsterCenterNameText"), (570.0, 908.0), (450.0, 70.0), 20)

# Detail hierarchy: larger primary name/HP, evenly spaced stats, and larger
# 2x2 skill targets for landscape phones.
canvas_bounds(widget("MonsterDetailNameText"), (1120.0, 151.0), (500.0, 78.0), 20)
font_size("MonsterDetailNameText", 52)
canvas_bounds(widget("MonsterDetailTypeText"), (1120.0, 224.0), (500.0, 48.0), 20)
font_size("MonsterDetailTypeText", 29)
canvas_bounds(widget("MonsterDetailHPLabel"), (1120.0, 301.0), (100.0, 58.0), 20)
font_size("MonsterDetailHPLabel", 34)
canvas_bounds(widget("MonsterDetailHPBar"), (1225.0, 305.0), (590.0, 50.0), 20)
canvas_bounds(widget("MonsterDetailHPText"), (1235.0, 309.0), (570.0, 42.0), 22)
font_size("MonsterDetailHPText", 28)
canvas_bounds(widget("MonsterDetailAPText"), (1135.0, 378.0), (320.0, 58.0), 20)
font_size("MonsterDetailAPText", 34)
canvas_bounds(widget("MonsterDetailSpeedText"), (1480.0, 378.0), (320.0, 58.0), 20)
font_size("MonsterDetailSpeedText", 34)
canvas_bounds(widget("MonsterSkillHeading"), (1130.0, 454.0), (680.0, 54.0), 20)
font_size("MonsterSkillHeading", 33)

for index in range(4):
    x = 1125.0 + 350.0 * (index % 2)
    y = 518.0 + 144.0 * (index // 2)
    canvas_bounds(widget(f"MonsterSkillBox_{index}"), (x, y), (330.0, 126.0), 20)
    canvas_bounds(widget(f"MonsterSkillName_{index}"), (x + 18.0, y + 34.0), (294.0, 58.0), 22)
    font_size(f"MonsterSkillName_{index}", 28)

canvas_bounds(widget("MonsterStatusHeading"), (1130.0, 816.0), (680.0, 54.0), 20)
font_size("MonsterStatusHeading", 33)
canvas_bounds(widget("MonsterStatusText_0"), (1135.0, 878.0), (320.0, 70.0), 20)
font_size("MonsterStatusText_0", 30)
canvas_bounds(widget("MonsterStatusText_1"), (1480.0, 878.0), (320.0, 70.0), 20)
font_size("MonsterStatusText_1", 30)

unreal.BlueprintEditorLibrary.compile_blueprint(asset)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {ASSET_PATH}")

unreal.log(
    "RD_MONSTER_TAB_RETUNE success layout=mobile-v2 "
    "rows=448x160 portrait=530x685 hp=590x50 skills=330x126 back=in-detail"
)
